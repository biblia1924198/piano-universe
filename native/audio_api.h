#pragma once
typedef struct AAudioStream AAudioStream;
typedef struct AAudioStreamBuilder AAudioStreamBuilder;
typedef int (*PrData)(AAudioStream*,void*,void*,int);
typedef void (*PrError)(AAudioStream*,void*,int);
typedef struct {
 int (*create)(AAudioStreamBuilder**);int (*destroy)(AAudioStreamBuilder*);
 void (*device)(AAudioStreamBuilder*,int);void (*direction)(AAudioStreamBuilder*,int);
 void (*sharing)(AAudioStreamBuilder*,int);void (*performance)(AAudioStreamBuilder*,int);
 void (*channels)(AAudioStreamBuilder*,int);void (*format)(AAudioStreamBuilder*,int);
 void (*sampleRate)(AAudioStreamBuilder*,int);int (*readInput)(AAudioStream*,void*,int,long long);void (*inputPreset)(AAudioStreamBuilder*,int);
 void (*usage)(AAudioStreamBuilder*,int);void (*data)(AAudioStreamBuilder*,PrData,void*);void (*error)(AAudioStreamBuilder*,PrError,void*);
 int (*open)(AAudioStreamBuilder*,AAudioStream**);int (*close)(AAudioStream*);int (*start)(AAudioStream*);int (*stop)(AAudioStream*);
 int (*rate)(AAudioStream*);int (*size)(AAudioStream*);int (*capacity)(AAudioStream*);int (*burst)(AAudioStream*);int (*mode)(AAudioStream*);int (*perf)(AAudioStream*);int (*xruns)(AAudioStream*);int (*id)(AAudioStream*);int (*ch)(AAudioStream*);int (*fmt)(AAudioStream*);int (*buffer)(AAudioStream*,int);
} PrApi;

#ifdef __cplusplus
extern "C" {
#endif
int pr_oboe_api(PrApi*);
#ifdef __cplusplus
}
#endif
