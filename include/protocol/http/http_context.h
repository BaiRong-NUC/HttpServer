#pragma once
#include <utils/public.h>
#include <utils/utils.h>
#include <protocol/http/http_request.h>
#include <protocol/http/http_response.h>
#include <utils/buffer.h>

#define MAX_LINE_SIZE 8192

/**
 * HTTP请求上下文管理，保存请求和响应对象，处理请求生命周期，提供接口供业务处理使用。
 * 包括:
 *      - 当前协议接受状态
 *          - 接受请求行阶段
 *          - 接受请求头部阶段
 *          - 接受请求正文阶段
 *          - 接受完毕阶段
 *          - 接受错误
 *      - 响应状态码: 在请求接受并且处理中,可能会有各种不同的问题.
 *      - 已经接受并处理的请求信息
 *      - 接受并处理数据
 *          - 接受/处理请求行
 *          - 接受/处理头部
 *          - 接受正文
 *          - 返回解析完毕的请求信息进行业务处理
 *          - 返回响应状态码?
 *          - 返回接受解析状态
 */

enum class HttpAcceptStatus
{
    ACCEPTING_ERROR,         // 接受错误
    ACCEPTING_REQUEST_LINE,  // 接受请求行阶段
    ACCEPTING_HEADERS,       // 接受请求头部阶段
    ACCEPTING_BODY,          // 接受请求正文阶段
    ACCEPTED                 // 接受完毕阶段
};

class HttpContext
{
   private:
    // 响应状态码,在请求接受并且处理中,可能会有各种不同的问题,需要记录响应状态码以便后续处理
    int _response_status;
    // 当前协议接受状态,在请求接受过程中会经历不同的阶段,需要记录当前阶段以便正确处理请求数据
    HttpAcceptStatus _accept_status;
    // 解析完成的Http请求信息
    HttpRequest _request;
    // 解析完成Http响应信息
    HttpResponse _response;

   public:
    HttpContext();

    int GetResponseStatus() const;

    HttpAcceptStatus GetAcceptStatus() const;

    HttpRequest &GetRequest();

    HttpResponse &GetResponse();

    // 获取Http请求并解析
    bool RecvRequest(Buffer &buffer);
};