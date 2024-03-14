// HTTP服务器
// 声明业务服务类
#pragma once
#include <acl-lib/acl_cpp/lib_acl.hpp>

// 业务服务类
class service_c: public acl::HttpServlet {
public:
	service_c(acl::socket_stream* conn, acl::session* sess);

protected:
	bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
	bool doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
	bool doOptions(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
	bool doError(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
	bool doOther(acl::HttpServletRequest& req, acl::HttpServletResponse& res, char const* method);

private:
	// 处理文件路由
	bool files(acl::HttpServletRequest& req, acl::HttpServletResponse& res);
};
