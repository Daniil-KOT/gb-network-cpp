#pragma once

#include <QString>

struct QueryHeader
{
    QString statusLine_;
    QString userAgent_;
    QString host_;
    QString contentType_;
    QString contentLength_;
    QString acceptLanguage_;
    QString acceptEncoding_;
    QString connection_;
};
