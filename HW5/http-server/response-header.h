#pragma once

#include <QString>

struct ResponseHeader
{
    QString statusLine_;
    QString date_;
    QString contentLength_;
    QString contentType_;
    QString connectionStatus_;
};
