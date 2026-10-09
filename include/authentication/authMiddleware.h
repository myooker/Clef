#ifndef CLEF_AUTHMIDDLEWARE_H
#define CLEF_AUTHMIDDLEWARE_H

#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/logging.h>

namespace clef {
    class AuthMiddleware {
    private:

    public:
        struct context
        {};

        void before_handle(crow::request &req, crow::response &res, context &ctx) {
            if (req.url == "/api/auth/login" && req.method == crow::HTTPMethod::POST) {
                return;
            }
            if (req.url == "/api/auth/signup" && req.method == crow::HTTPMethod::POST) {
                return;
            }
            // here we need to check users session
            return;
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx) {
        }
    };
}

#endif // CLEF_AUTHMIDDLEWARE_H
