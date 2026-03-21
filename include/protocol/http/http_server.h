#pragma once
#include <utils/public.h>
#include <utils/utils.h>
#include <protocol/http/http_context.h>
#include <protocol/http/http_response.h>
#include <server/tcp_server.h>

/**
 * 功能:
 * 1. GET 請求處理函數的映射表,string是正則表達式
 * 2. POST 請求的路由映射表
 * 3. PUT 請求的路由映射表
 * 4. DELETE 請求的路由映射表
 * 5. 靜態資源相對根目錄
 * 6. TCP 服務器進行鏈接IO操作
 * 流程:
 * 1. 從socket接收HTTP請求數據
 * 2. 調用OnMessage回調函數處理請求數據
 * 3. 對請求進行解析,得到HttpRequest,HttpResponse對象
 * 4. 請求路由查找
 *      - 靜態資源請求: 根據URI查找對應的文件,讀取文件內容作為響應正文,設置Content-Type等響應頭部字段
 *      - 功能請求: 根據URI查找對應的處理函數,調用函數處理請求,得到響應內容和狀態碼,設置HttpResponse對象
 * 5. 將HttpResponse對象組織成http格式進行發送
 */

class HttpServer
{
   private:
    using HandlerFunc = std::function<void(const HttpRequest &, HttpResponse &)>;
    std::unordered_map<std::string, HandlerFunc> _get_handlers;                   // GET
    std::unordered_map<std::string, HandlerFunc> _post_handlers;                  // POST
    std::unordered_map<std::string, HandlerFunc> _put_handlers;                   // PUT
    std::unordered_map<std::string, HandlerFunc> _delete_handlers;                // DELETE
    TcpServer _tcp_server;                                                        // TCP服務器
    void _OnMessage(const PtrConnection &conn, Buffer &buffer);                   // 處理請求數據的回調函數
    void _OnConnected();                                                          // 設置tcp上下文
    HandlerFunc _FindHandler(const std::string &method, const std::string &uri);  // 查找處理函數
   public:
    const std::string static_root;  // 靜態資源根目錄
    HttpServer();
    // uri_pattern是正則表達式,用於匹配請求URI,handler是處理函數,接受HttpRequest對象和HttpResponse對象參數,用於處理請求並設置響應內容
    void Get(const std::string &uri_pattern, HandlerFunc handler);
    void Post(const std::string &uri_pattern, HandlerFunc handler);
    void Put(const std::string &uri_pattern, HandlerFunc handler);
    void Delete(const std::string &uri_pattern, HandlerFunc handler);

    // 設置超時時間,以s為單位,超時後自動關閉不活躍的連接
    void SetInactiveTimeout(int timeout);

    // 啟動服務器,開始接受和處理請求
    void Listen();
};