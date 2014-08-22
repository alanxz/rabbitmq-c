#ifndef LIGHT_STREAMS_H
#define LIGHT_STREAMS_H

#include <stdint.h>
#include <stdlib.h>

typedef enum ls_status_enum {
  LS_STATUS_OK =                         0x0,     /**< Operation successful */
  LS_GENERAL_ERROR =                    -0x0001,   /**< An error occurred */
  LS_MAILBOX_GET_TIMEOUT =              -0x0002,   /**< Timeout waiting form mailbox */
  LS_SEND_WITH_NON_ZERO_BUFFER_LEN =    -0x0003,
  LS_SEND_AND_NOT_OPEN_BEFORE_POST =    -0x0004,
  LS_SEND_AND_NOT_OPEN_ON_GET =         -0x0005,
  LS_SEND_GET_ERROR =                   -0x0006,
  LS_AVAILABLE_AND_NOT_OPEN =           -0x0007,
  LS_TOOK_AND_NOT_OPEN =                -0x0008,
  LS_SEND_AND_CLOSED_BEFORE_POST =      -0x0009,
  LS_TIMEOUT_WAITING_FOR_CLOSE  =       -0x000A,
} ls_status_t;


typedef enum ls_state_enum {
  LS_STATE_MESSAGE_OPEN           = 0x0,
  LS_STATE_MESSAGE_CLOSED         = 0x1,
  LS_STATE_MESSAGE_SENDER_ABORT   = 0x2,
  LS_STATE_MESSAGE_RECEIVER_ABORT = 0x3
} ls_state_t;
struct lightStreamAggregate_s;

typedef struct lightStreamAggregate_s * lightStreamAggregateP_t;

struct lightStreamMailBoxPub_s;

typedef struct lightStreamMailBoxPub_s * lightStreamMailBoxPubP_t;

struct lightStreamMailBoxInfo_s {
  uint32_t timeOutMs;
  lightStreamMailBoxPubP_t mailBox;
};

typedef struct lightStreamMailBoxInfo_s *lightStreamMailBoxInfoP_t;

struct lightStreamSocketSetup_s {
  uint32_t toRxMailBoxGetTimeoutMs;
  uint32_t toTxMailBoxGetTimeoutMs;
  char const * const rxName;
  char const * const txName;
};

typedef struct lightStreamSocketSetup_s const *lightStreamSocketSetupP_t;

typedef int (*socketLs_fn_t)(lightStreamAggregateP_t lsAggP, const lightStreamSocketSetupP_t setupInfoP);
typedef size_t (*lenLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef int (*setLenLs_fn_t)(lightStreamAggregateP_t lsAggP, size_t bufferLen);
typedef int (*sendLs_fn_t)(lightStreamAggregateP_t lsAggP, const char *bufferPtr, size_t bufferLen);
typedef int (*availableLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef const char *(*peekLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef int (*tookBytesLs_fn_t)(lightStreamAggregateP_t lsAggP, size_t lenTook);
typedef void (*openMessageLs_fn_t)(lightStreamAggregateP_t lsAggP, size_t len);
typedef void (*closeMessageLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef void (*senderAbortMessageLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef int (*senderWaitForClose_fn_t)(lightStreamAggregateP_t lsAggP);
typedef void (*receiverAbortMessageLs_fn_t)(lightStreamAggregateP_t lsAggP);
typedef lightStreamMailBoxPubP_t (*makeMailBox_fn_t)(lightStreamAggregateP_t lsAggP, uint32_t timeOutMs, const char *nameStr);
typedef int (*postToMailBox_fn_t)(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);
typedef int (*getFromMailBox_fn_t)(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);
typedef int (*emptyMailBox_fn_t)(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);

struct lightStream_class_s {
  socketLs_fn_t socketFn;
  setLenLs_fn_t setLenFn;
  sendLs_fn_t sendFn;
  lenLs_fn_t lenFn;
  availableLs_fn_t availableFn;
  peekLs_fn_t peekFn;
  tookBytesLs_fn_t tookBytesFn;
  openMessageLs_fn_t openMessageFn;
  closeMessageLs_fn_t closeMessageFn;
  senderAbortMessageLs_fn_t senderAbortMessageFn;
  senderWaitForClose_fn_t senderWaitForCloseFn;
  receiverAbortMessageLs_fn_t receiverAbortMessageFn;
  makeMailBox_fn_t makeMailBoxFn;
  postToMailBox_fn_t postToMailBoxFn;
  getFromMailBox_fn_t getFromMailBoxFn;
  emptyMailBox_fn_t emptyMailBoxFn;
};

struct lightStreamAggregate_s {
  const struct lightStream_class_s *klassP;
  void *userStateP;
  volatile ls_state_t messageState;
  volatile size_t len;
  volatile size_t bytesSent;
  const char * volatile bufferPtr;
  volatile size_t bufferLen;
  struct lightStreamMailBoxInfo_s toRxerMailBoxInfo;
  struct lightStreamMailBoxInfo_s toTxerMailBoxInfo;
};

int lsSocketCommon(lightStreamAggregateP_t lsAggP, const lightStreamSocketSetupP_t setupInfoP);
int lsSetLenCommon(lightStreamAggregateP_t lsAggP, size_t len);
size_t lsLenCommon(lightStreamAggregateP_t lsAggP);
int lsSendCommon(lightStreamAggregateP_t lsAggP, const char *bufferPtr, size_t bufferLen);
int lsAvailableCommon(lightStreamAggregateP_t lsAggP);
const char *lsPeekCommon(lightStreamAggregateP_t lsAggP);
int lsTookBytesCommon(lightStreamAggregateP_t lsAggP, size_t tookLen);
void lsOpenMessageCommon(lightStreamAggregateP_t lsAggP, size_t len);
void lsCloseMessageCommon(lightStreamAggregateP_t lsAggP);
void lsSenderAbortMessageCommon(lightStreamAggregateP_t lsAggP);
int lsSenderWaitForCloseCommon(lightStreamAggregateP_t lsAggP);
void lsReceiverAbortMessageCommon(lightStreamAggregateP_t lsAggP);




int lsSocket(lightStreamAggregateP_t lsAggP, lightStreamSocketSetupP_t setupInfoP);
int lsSetLen(lightStreamAggregateP_t lsAggP, size_t len); //todo deprecate this...
size_t lsLen(lightStreamAggregateP_t lsAggP);
int lsSend(lightStreamAggregateP_t lsAggP, const char *bufferPtr, size_t bufferLen);
int lsAvailable(lightStreamAggregateP_t lsAggP);
const char *lsPeek(lightStreamAggregateP_t lsAggP);
int lsTookBytes(lightStreamAggregateP_t lsAggP, size_t lenTook);
void lsOpenMessage(lightStreamAggregateP_t lsAggP, size_t len);
void lsCloseMessage(lightStreamAggregateP_t lsAggP);
void lsSenderAbortMessage(lightStreamAggregateP_t lsAggP);
int lsSenderWaitForClose(lightStreamAggregateP_t lsAggP);
void lsReceiverAbortMessage(lightStreamAggregateP_t lsAggP);

lightStreamMailBoxPubP_t lsMakeMailBox(lightStreamAggregateP_t lsAggP, uint32_t timeOutMs, const char *nameStr);
int lsPostToMailBox(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);
int lsPostToToTxMailBox(lightStreamAggregateP_t lsAggP);
int lsPostToToRxMailBox(lightStreamAggregateP_t lsAggP);

int lsGetFromMailBox(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);
int lsEmptyMailBox(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP);

#endif
