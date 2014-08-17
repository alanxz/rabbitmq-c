#include "lightStreams.h"

#include <stdlib.h>
#include <assert.h>

int lsSenderAbort(lightStreamAggregateP_t lsAggP, ls_status_t error)
{
  lsAggP->messageState = LS_STATE_MESSAGE_SENDER_ABORT;
  lsPostToMailBox(lsAggP, &lsAggP->toRxerMailBoxInfo);
  return (int) error;
}

int lsReceiverAbort(lightStreamAggregateP_t lsAggP, ls_status_t error)
{
  lsAggP->messageState = LS_STATE_MESSAGE_RECEIVER_ABORT;
  lsPostToMailBox(lsAggP, &lsAggP->toTxerMailBoxInfo);
  return (int) error;
}


int lsSocketCommon(lightStreamAggregateP_t lsAggP, lightStreamSocketSetupP_t setupInfoP)
{
  assert(lsAggP);
  lsAggP->toRxerMailBoxInfo.timeOutMs = setupInfoP->toRxMailBoxGetTimeoutMs;
  lsAggP->toRxerMailBoxInfo.mailBox = lsMakeMailBox(lsAggP, setupInfoP->toRxMailBoxGetTimeoutMs, setupInfoP->rxName);
  assert(lsAggP->toRxerMailBoxInfo.mailBox);

  lsAggP->toTxerMailBoxInfo.timeOutMs = setupInfoP->toTxMailBoxGetTimeoutMs;
  lsAggP->toTxerMailBoxInfo.mailBox = lsMakeMailBox(lsAggP, setupInfoP->toTxMailBoxGetTimeoutMs, setupInfoP->txName);
  assert(lsAggP->toTxerMailBoxInfo.mailBox);

  lsAggP->messageState = LS_STATE_MESSAGE_CLOSED;
  lsAggP->bufferLen = 0;
  lsAggP->bufferPtr = NULL;
  lsAggP->len = 0;

  return LS_STATUS_OK;
}

int lsSetLenCommon(lightStreamAggregateP_t lsAggP, size_t len)
{
  lsAggP->len = len;

  return LS_STATUS_OK;
}

size_t lsLenCommon(lightStreamAggregateP_t lsAggP)
{
  return lsAggP->len;
}

int lsSendCommon(lightStreamAggregateP_t lsAggP, const char *bufferPtr, size_t bufferLen)
{
  if (lsAggP->bufferLen) return lsSenderAbort(lsAggP, LS_SEND_WITH_NON_ZERO_BUFFER_LEN);
  if (LS_STATE_MESSAGE_OPEN!=lsAggP->messageState) return lsSenderAbort(lsAggP, LS_SEND_AND_NOT_OPEN_BEFORE_POST);

  lsAggP->bufferPtr = bufferPtr;
  lsAggP->bufferLen = bufferLen;

  int result = lsPostToMailBox(lsAggP, &lsAggP->toRxerMailBoxInfo);
  if (LS_STATUS_OK != result) return result;

  result = lsGetFromMailBox(lsAggP, &lsAggP->toTxerMailBoxInfo);
  if (LS_STATUS_OK != result) return lsSenderAbort(lsAggP, result);

  if (LS_STATE_MESSAGE_OPEN!=lsAggP->messageState) return LS_SEND_AND_NOT_OPEN_ON_GET;

  return LS_STATUS_OK;
}

int lsAvailableCommon(lightStreamAggregateP_t lsAggP)
{
  while ((LS_STATE_MESSAGE_OPEN==lsAggP->messageState) && (!lsAggP->bufferLen)) {
    int result = lsGetFromMailBox(lsAggP, &lsAggP->toRxerMailBoxInfo);
    if (LS_STATUS_OK != result) return result;
  }

  if (LS_STATE_MESSAGE_OPEN!=lsAggP->messageState) return LS_AVAILABLE_AND_NOT_OPEN;

  return lsAggP->bufferLen;
}

const char *lsPeekCommon(lightStreamAggregateP_t lsAggP)
{
  if ((LS_STATE_MESSAGE_OPEN==lsAggP->messageState) && lsAggP->bufferPtr && lsAggP->bufferLen) {
    return lsAggP->bufferPtr;
  } else {
    lsAggP->messageState = LS_STATE_MESSAGE_RECEIVER_ABORT;
    return NULL;
  }
}

int lsTookBytesCommon(lightStreamAggregateP_t lsAggP, size_t tookLen)
{
  if (LS_STATE_MESSAGE_OPEN!=lsAggP->messageState) return lsReceiverAbort(lsAggP,LS_TOOK_AND_NOT_OPEN);

  assert(lsAggP->bufferPtr);
  assert(lsAggP->bufferLen);
  assert(lsAggP->bufferLen >= tookLen);

  lsAggP->bufferLen -= tookLen;
  lsAggP->bufferPtr += tookLen;

  if (!lsAggP->bufferLen) {
    lsAggP->bufferPtr = NULL;
    int result = lsPostToMailBox(lsAggP, &lsAggP->toTxerMailBoxInfo);
    if (LS_STATUS_OK != result) {
      lsAggP->messageState = LS_STATE_MESSAGE_RECEIVER_ABORT;
      return result;
    }
  }

  return LS_STATUS_OK;
}

void lsOpenMessageCommon(lightStreamAggregateP_t lsAggP, size_t len)
{
  lsAggP->len = len;
  lsAggP->bufferPtr = NULL;
  lsAggP->bufferLen = 0;
  lsAggP->messageState = LS_STATE_MESSAGE_OPEN;
}

void lsCloseMessageCommon(lightStreamAggregateP_t lsAggP)
{
  lsAggP->messageState = LS_STATE_MESSAGE_CLOSED;

}

void lsSenderAbortMessageCommon(lightStreamAggregateP_t lsAggP)
{
  if (LS_STATE_MESSAGE_OPEN == lsAggP->messageState) {
    lsAggP->messageState = LS_STATE_MESSAGE_SENDER_ABORT;
  }
}

void lsReceiverAbortMessageCommon(lightStreamAggregateP_t lsAggP)
{
  if (LS_STATE_MESSAGE_OPEN == lsAggP->messageState) {
    lsAggP->messageState = LS_STATE_MESSAGE_RECEIVER_ABORT;
  }
}


int lsSocket(lightStreamAggregateP_t lsAggP, lightStreamSocketSetupP_t setupInfoP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->socketFn);
  return lsAggP->klassP->socketFn(lsAggP,setupInfoP);
 }

int lsSetLen(lightStreamAggregateP_t lsAggP, size_t len)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->setLenFn);
  return lsAggP->klassP->setLenFn(lsAggP,len);
}

size_t lsLen(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->lenFn);
  return lsAggP->klassP->lenFn(lsAggP);
}

int lsSend(lightStreamAggregateP_t lsAggP, const char *bufferPtr, size_t bufferLen)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->sendFn);
  return lsAggP->klassP->sendFn(lsAggP,bufferPtr,bufferLen);
}

int lsAvailable(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->availableFn);
  return lsAggP->klassP->availableFn(lsAggP);
}

const char *lsPeek(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->peekFn);
  return lsAggP->klassP->peekFn(lsAggP);
}

int lsTookBytes(lightStreamAggregateP_t lsAggP, size_t tookLen)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->tookBytesFn);
  return lsAggP->klassP->tookBytesFn(lsAggP, tookLen);
}

void lsOpenMessage(lightStreamAggregateP_t lsAggP, size_t len)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->openMessageFn);
  return lsAggP->klassP->openMessageFn(lsAggP, len);
}

void lsCloseMessage(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->closeMessageFn);
  return lsAggP->klassP->closeMessageFn(lsAggP);
}

void lsSenderAbortMessage(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->senderAbortMessageFn);
  return lsAggP->klassP->senderAbortMessageFn(lsAggP);
}

void lsReceiverAbortMessage(lightStreamAggregateP_t lsAggP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->receiverAbortMessageFn);
  return lsAggP->klassP->receiverAbortMessageFn(lsAggP);
}

lightStreamMailBoxPubP_t lsMakeMailBox(lightStreamAggregateP_t lsAggP, uint32_t timeOutMs, const char *nameStr)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->makeMailBoxFn);
  return lsAggP->klassP->makeMailBoxFn(lsAggP, timeOutMs, nameStr);
}

int lsPostToMailBox(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->postToMailBoxFn);
  return lsAggP->klassP->postToMailBoxFn(lsAggP, mailBoxInfoP);
}

int lsGetFromMailBox(lightStreamAggregateP_t lsAggP, lightStreamMailBoxInfoP_t mailBoxInfoP)
{
  assert(lsAggP);
  assert(lsAggP->klassP);
  assert(lsAggP->klassP->getFromMailBoxFn);
  return lsAggP->klassP->getFromMailBoxFn(lsAggP, mailBoxInfoP);
}

