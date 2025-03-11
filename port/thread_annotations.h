#ifndef STORAGE_LEVELDB_PORT_THREAD_ANNOTATIONS_H_
#define STORAGE_LEVELDB_PORT_THREAD_ANNITATIONS_H_

#if !defined(THREAD_ANNOTATION_ATTRIBUTE__)

#if defined(__clang__)

#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#endif  // !defined(THREAD_ANNOTATION_ATTRIBUTE__)

#ifndef GUARDED_BY
#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))  //ATTRIBUTE__两个'_'
#endif

#endif