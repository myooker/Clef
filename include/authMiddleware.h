#ifndef CLEF_AUTHMIDDLEWARE_H
#define CLEF_AUTHMIDDLEWARE_H

#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/logging.h>

namespace clef {
    struct AuthMiddleware {
        struct context
        {};

        void before_handle(crow::request &req, crow::response &res, context &ctx) {
            if (req.url == "/api/auth/login" && req.method == crow::HTTPMethod::POST) {
                return;
            }
            return;
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx) {
        }
    };
}

#endif // CLEF_AUTHMIDDLEWARE_H
