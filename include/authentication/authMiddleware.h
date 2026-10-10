#ifndef CLEF_AUTHMIDDLEWARE_H
#define CLEF_AUTHMIDDLEWARE_H

#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/logging.h>
#include "sessionStore.h"

namespace clef {
    class AuthMiddleware {
    private:
        SessionStore *m_store { nullptr };
    public:
        struct context
        {};

        template<typename AllContext>
        void before_handle(crow::request &req, crow::response &res, context &ctx, AllContext &allCtx) {
            if (req.method == crow::HTTPMethod::POST ||
               (req.url == "/api/auth/signup" || req.url == "/api/auth/login"))
                return;

            auto& cookies = allCtx.template get<crow::CookieParser>();
            std::string token = cookies.get_cookie("clef_session");

            if (token.empty() || !m_store->validateSession(token)) {
                res.code = 401;
                res.end();
                return;
            }
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx) {}
        void setSessionStore(SessionStore *store) { m_store = store; }
    };
}

#endif // CLEF_AUTHMIDDLEWARE_H
