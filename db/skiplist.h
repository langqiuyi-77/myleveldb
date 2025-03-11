#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_


#include <atomic>
#include <cassert>

// 线程安全
// ------------------
//
// 写操作要求单独外部的同步机制，常常是mutex，
// 读操作要求保证SkipList不会被销毁掉，在读操作正在进行的时候
// 除此之外，读操作不会有任何内部的加锁或者同步机制
//
// Invariants:
// 
// (1) 被分配了的node是不会被删除的，直到整个skipList被销毁掉的时候
//
// (2) Node的内容除了next/prev pointes部分，在链接到skipList后
// 是不可以被修改的. 只有Insert()修改这个list, 而且它的初始化是通过
// release-store保护的

// 简化实现 TODO:
// ------------------
// 
// (1) arena_ 内存池，它一次性的申请一大块内存，然后在这个小块上进行小
// 块分配，减少　malloc/free 的开销，适应于SkipList这种高频插入但是较
// 少删除的数据结构
//
// (2) Random 随机数生成
// 
// (3) iterator


// SkipList是模板类，模板类实现不能是.cc分开的
// 另外SkipList是内部数据结构，不需要放在include对外暴露
namespace leveldb {

// 在实现的时候可以传递一个自定义的比较器而不是使用key默认的比较逻辑，灵活性
// struct Comparator {
//   int operator()(const Key& a, const Key& b) const {
//     if (a < b) {
//       return -1;
//     } else if (a > b) {
//       return +1;
//     } else {
//       return 0;
//     }
//   }
// };
template <typename Key, class Comparator>
class SkipList
{
private:
  struct Node;  // Node的前向声明，前向声明在SkipLisyt的private中，其完整定义也必须在SkipList内部中

public:
  explicit SkipList(Comparator cmp);  // 禁止隐式转换

  // SkipList禁止复制功能，设计一个类的时候一开始就要有这样的思考
  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

  // 插入一个key到list当中
  // 要求：不能插入相同的key SkipList没有delete操作
  void Insert(const Key& key);

  // 返回是否有一个相同的key在list中
  bool Contains(const Key& key) const;

private:
  enum { kMaxHeight = 12 };   // enum确保kMaxHeight是一个纯编译时常量，不会占用额外内存，类似于＃define, C++11后使用static constexpr int kMaxHeight = 12;

  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relexed);  // 可以获得stale version
    
  }
  Node* NewNode(const Key& key, int height);  // 创建一个next_[]数组有height个的Node对象
  bool KeyIsAfterNode(const Key& key, Node* n) const;
  Node* FindGreaterOrEqual(const Key& key, Node** prev) const;
  int RandomHeight() const;
  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

  Comparator const compare_;

  Node* const head_;

  // 只会被Insert()修改，读操作可能会有竞争，但是stale value是可以的
  std::atomic<int> max_height_;

};

// 在SkipList内部的Node实现
template <typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node {
  explicit Node(const Key& k) : key(k) {}

  Key const key;

  // 用于链接的修改器或者访问器，包装在方法中使得我们可以增加合适的限制
  Node* Next(int n) {
    assert(n >= 0);
    // 使用　acquire load 使得我们不会观察到没有初始化完成的数据
    // 内存屏障中常常使用　acquire 和　release
    return next_[n].load(std::memory_order_acquire);
  }
  void SetNext(int n, Node* x) {
    assert(n >= 0);
    // 使用　release order 使得当我们修改之后其他的线程能够看见
    // atomic变量本身保证原子性，通过设定　memeory_order 限制重排序，可见性
    next[n].store(std::memory_order_release);
  }

  // No-barrier variants that can be safely used in a few locations.
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_relaxed);
  }
  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_relaxed);
  }
  
  private:
    // 保存这个node不同level下的next指针　
    std::atomic<Node*> next_[1];
};

template<typename Key, class Comparator>
// SkipList<Key, Comparator>::Node* 内部类，编译器不知道这个是一个内部的类型还是一个变量，使用typename告诉编译器这是一个类型
typename SkipList<Key, Comparator>::Node* 
SkipList<Key, Comparator>::NewNode(const Key& key, int height) {
  char* const node_memory = (char*)malloc(sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
  
  // malloc 分配可能失败
  if (!node_memory) {
    printf("malloc 分配空间失败\n");
    return nullptr;
  }

  // malloc 分配的空间是没有初始化的
  return new (node_memory) Node<Key>(key);
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* 
SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {  
      // Keep searching in this level
      x = next;
    } else /* next->key 大于或者等于 key　*/ {  
      if (prev != nullptr) prev[level] = x; // 于是　x 就是最后一个小于　key　的节点
      if (level == 0) {
        return next;
      } else {
        // Switch to next list
        level --;
      }
    }
  } 
}

template<typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() const {
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kMaxHeight && (rand() % 4 == 0)) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

// SkipList的具体实现
template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp) 
  : compare_(cmp),
    head_(NewNode(0 /* any key will do */, kMaxHeight)),
    max_height_(1) {
  // head_是一个有全level节点的跳表头节点
  for (int i = 0; i < KMaxHeight; i ++) {
    head_->SetNext(i, nullptr);
  }
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key) {
  Node* prev[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, prev);

  // Not allow duplicate insertion
  assert(x == nullptr || !Equal(key, x->key));

  int height = RandomeHeight();
  if (height > GetMaxHeight()) {  // 超出MaxHeight的部分使用head_
    for (int i = GetMaxHeight(); i < height; i ++) {
      prev[i] = head_;
    }
    // 对max_height的修改可以没有什么同步要求
    max_height_.store(height, std::memory_order_relaxed);
  }
  
  x = NewNode(key, height);
  for (int i = 0; i < height; i ++) {
    // 在这里使用NoBarrier读是可以的因为只有这一个在写，noBarrier写也ok
    // 因为后面对prev[i]设置的时候是会有屏障的,能保证在prev．SetNext的时候
    // x的next是设置好了的
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    // 重点就是维持两种一致性：
    // (1) 保证在加入到SkipList的时候next指针已经指向后面的
    // (2) 保证如果上层看见了的话，下层一定要存在
    prev[i]->SetNext(i, x); 
  }
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const {
  Node* x = FindGreaterOrEqual(key, nullptr); // 看是否存在，level0
  if (x != nullptr && Equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

} // namespace leveldb

#endif