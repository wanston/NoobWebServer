//
// Created by tong on 19-2-22.
//

#include "HttpChannel.h"
#include "Log.h"

void HttpChannel::__handleReadEvent() {
    LOG << "read fd " << __fd;
}


void HttpChannel::__handleWriteEvent() {
    LOG << "write fd" << __fd;
}