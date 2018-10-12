---
title: go源码分析-http处理流程分析
date: 2017-10-31 14:39:54
tags: go.source.code
---
## 【概述】


GO的web服务器的简单helloword 服务器就是使用net/http库的进行编写。需要看一下http.ListenAndServe的工作原理。

    package main
    import "net/http"
    func main() {
        http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
            w.Write([]byte(`hello world`))
        })
        http.ListenAndServe(":3000", nil)
    }

主要包括：

+ uri_path跟handler的对应关系是如何初始化的
+ 端口接收到请求后，请求是如何处理、以及如何被定义的func处理、响应是如何处理的

## HandleFunc的参数

HandleFunc的第二个参数是func(ResponseWriter, *Request)，会赋值给HandlerFunc类型

    // The HandlerFunc type is an adapter to allow the use of
    // ordinary functions as HTTP handlers.  If f is a function
    // with the appropriate signature, HandlerFunc(f) is a
    // Handler that calls f.
    type HandlerFunc func(ResponseWriter, *Request)
    
    // ServeHTTP calls f(w, r).
    func (f HandlerFunc) ServeHTTP(w ResponseWriter, r *Request) {
    	f(w, r)
    }
## 映射处理

在src/net/http/server.go中使用`HandleFunc`进行注册，会将其放入多路复用的结构体`ServeMux`中。

src/net/http/server.go

    // HandleFunc registers the handler function for the given pattern.
    func (mux *ServeMux) HandleFunc(pattern string, handler func(ResponseWriter, *Request)) {
        mux.Handle(pattern, HandlerFunc(handler))
    }
    // Handle registers the handler for the given pattern
    // in the DefaultServeMux.
    // The documentation for ServeMux explains how patterns are matched.
    func Handle(pattern string, handler Handler) { DefaultServeMux.Handle(pattern, handler) }
    // HandleFunc registers the handler function for the given pattern
    // in the DefaultServeMux.
    // The documentation for ServeMux explains how patterns are matched.
    func HandleFunc(pattern string, handler func(ResponseWriter, *Request)) {
        DefaultServeMux.HandleFunc(pattern, handler)
    }
 
     // HandleFunc registers the handler function for the given pattern.
    func (mux *ServeMux) HandleFunc(pattern string, handler func(ResponseWriter, *Request)) {
    	mux.Handle(pattern, HandlerFunc(handler))
    }
    
    type Handler interface {
	   ServeHTTP(ResponseWriter, *Request)
    }
    
    func (mux *ServeMux) Handle(pattern string, handler Handler) {
    }
    
    // The HandlerFunc type is an adapter to allow the use of
    // ordinary functions as HTTP handlers.  If f is a function
    // with the appropriate signature, HandlerFunc(f) is a
    // Handler that calls f.
    type HandlerFunc func(ResponseWriter, *Request)
 
http请求的多路复用结构体`ServeMux`
   
    // ServeMux is an HTTP request multiplexer.
    // It matches the URL of each incoming request against a list of registered
    // patterns and calls the handler for the pattern that
    // most closely matches the URL.
    //
    // Patterns name fixed, rooted paths, like "/favicon.ico",
    // or rooted subtrees, like "/images/" (note the trailing slash).
    // Longer patterns take precedence over shorter ones, so that
    // if there are handlers registered for both "/images/"
    // and "/images/thumbnails/", the latter handler will be
    // called for paths beginning "/images/thumbnails/" and the
    // former will receive requests for any other paths in the
    // "/images/" subtree.
    //
    // Note that since a pattern ending in a slash names a rooted subtree,
    // the pattern "/" matches all paths not matched by other registered
    // patterns, not just the URL with Path == "/".
    //
    // If a subtree has been registered and a request is received naming the
    // subtree root without its trailing slash, ServeMux redirects that
    // request to the subtree root (adding the trailing slash). This behavior can
    // be overridden with a separate registration for the path without
    // the trailing slash. For example, registering "/images/" causes ServeMux
    // to redirect a request for "/images" to "/images/", unless "/images" has
    // been registered separately.
    //
    // Patterns may optionally begin with a host name, restricting matches to
    // URLs on that host only.  Host-specific patterns take precedence over
    // general patterns, so that a handler might register for the two patterns
    // "/codesearch" and "codesearch.google.com/" without also taking over
    // requests for "http://www.google.com/".
    //
    // ServeMux also takes care of sanitizing the URL request path,
    // redirecting any request containing . or .. elements or repeated slashes
    // to an equivalent, cleaner URL.
    type ServeMux struct {
    	mu    sync.RWMutex
    	m     map[string]muxEntry
    	hosts bool // whether any patterns contain hostnames
    }
    
## 对外提供服务


    // A Server defines parameters for running an HTTP server.
    // The zero value for Server is a valid configuration.
    type Server struct {
    	Addr           string        // TCP address to listen on, ":http" if empty
    	Handler        Handler       // handler to invoke, http.DefaultServeMux if nil
    	ReadTimeout    time.Duration // maximum duration before timing out read of the request
    	WriteTimeout   time.Duration // maximum duration before timing out write of the response
    	MaxHeaderBytes int           // maximum size of request headers, DefaultMaxHeaderBytes if 0
    	TLSConfig      *tls.Config   // optional TLS config, used by ListenAndServeTLS
    
    	// TLSNextProto optionally specifies a function to take over
    	// ownership of the provided TLS connection when an NPN
    	// protocol upgrade has occurred.  The map key is the protocol
    	// name negotiated. The Handler argument should be used to
    	// handle HTTP requests and will initialize the Request's TLS
    	// and RemoteAddr if not already set.  The connection is
    	// automatically closed when the function returns.
    	// If TLSNextProto is nil, HTTP/2 support is enabled automatically.
    	TLSNextProto map[string]func(*Server, *tls.Conn, Handler)
    
    	// ConnState specifies an optional callback function that is
    	// called when a client connection changes state. See the
    	// ConnState type and associated constants for details.
    	ConnState func(net.Conn, ConnState)
    
    	// ErrorLog specifies an optional logger for errors accepting
    	// connections and unexpected behavior from handlers.
    	// If nil, logging goes to os.Stderr via the log package's
    	// standard logger.
    	ErrorLog *log.Logger
    
    	disableKeepAlives int32     // accessed atomically.
    	nextProtoOnce     sync.Once // guards initialization of TLSNextProto in Serve
    	nextProtoErr      error
    }
    

其中`ListenAndServe`函数的逻辑，其中tcpKeepAliveListener是开启了tcp的keep-alive

    // ListenAndServe listens on the TCP network address srv.Addr and then
    // calls Serve to handle requests on incoming connections.
    // Accepted connections are configured to enable TCP keep-alives.
    // If srv.Addr is blank, ":http" is used.
    // ListenAndServe always returns a non-nil error.
    func (srv *Server) ListenAndServe() error {
    	addr := srv.Addr
    	if addr == "" {
    		addr = ":http"
    	}
    	ln, err := net.Listen("tcp", addr)
    	if err != nil {
    		return err
    	}
    	return srv.Serve(tcpKeepAliveListener{ln.(*net.TCPListener)}) 
    }



在Serve中在Accept处于等待，当请求到来时，Accept会从Listener l中接收一个到来的连接并创建一个新的goroutine处理请求。这个goroutine会读取请求，调用Server.Handler来处理他们。
    
    func (srv *Server) Serve(l net.Listener) error {
    	defer l.Close()
    	....
    	for {
    		rw, e := l.Accept()
    		if e != nil {
    			....
    			return e
    		}
    		tempDelay = 0
    		c := srv.newConn(rw)
    		c.setState(c.rwc, StateNew) // before Serve can return
    		go c.serve()
    	}
    }

其中newConn是获得一个新连接

    // Create new connection from rwc.
    func (srv *Server) newConn(rwc net.Conn) *conn {
    	c := &conn{
    		server: srv,
    		rwc:    rwc,
    	}
    	if debugServerConnections {
    		c.rwc = newLoggingConn("server", c.rwc)
    	}
    	return c
    }
    
其中conn结构体在net/http/server.go文件中


    // A conn represents the server side of an HTTP connection.
    type conn struct {
    	// server is the server on which the connection arrived.
    	// Immutable; never nil.
    	server *Server
    
    	// rwc is the underlying network connection.
    	// This is never wrapped by other types and is the value given out
    	// to CloseNotifier callers. It is usually of type *net.TCPConn or
    	// *tls.Conn.
    	rwc net.Conn
    
    	// remoteAddr is rwc.RemoteAddr().String(). It is not populated synchronously
    	// inside the Listener's Accept goroutine, as some implementations block.
    	// It is populated immediately inside the (*conn).serve goroutine.
    	// This is the value of a Handler's (*Request).RemoteAddr.
    	remoteAddr string
    
    	// tlsState is the TLS connection state when using TLS.
    	// nil means not TLS.
    	tlsState *tls.ConnectionState
    
    	// werr is set to the first write error to rwc.
    	// It is set via checkConnErrorWriter{w}, where bufw writes.
    	werr error
    
    	// r is bufr's read source. It's a wrapper around rwc that provides
    	// io.LimitedReader-style limiting (while reading request headers)
    	// and functionality to support CloseNotifier. See *connReader docs.
    	r *connReader
    
    	// bufr reads from r.
    	// Users of bufr must hold mu.
    	bufr *bufio.Reader
    
    	// bufw writes to checkConnErrorWriter{c}, which populates werr on error.
    	bufw *bufio.Writer
    
    	// lastMethod is the method of the most recent request
    	// on this connection, if any.
    	lastMethod string
    
    	// mu guards hijackedv, use of bufr, (*response).closeNotifyCh.
    	mu sync.Mutex
    
    	// hijackedv is whether this connection has been hijacked
    	// by a Handler with the Hijacker interface.
    	// It is guarded by mu.
    	hijackedv bool
    }
    



serve方法



    // Serve a new connection.
    func (c *conn) serve() {
    	c.remoteAddr = c.rwc.RemoteAddr().String()
    	defer func() {
    		... // defer处理
    	}()
    
    	... // tls connection
    
    	c.r = &connReader{r: c.rwc}
    	c.bufr = newBufioReader(c.r)
    	c.bufw = newBufioWriterSize(checkConnErrorWriter{c}, 4<<10)
    
    	for {
    		w, err := c.readRequest() //读取请求
    		...
    		if err != nil {
    			....
    			return
    		}
    
    		
    		...     // Expect 100 Continue support
    		
    		// HTTP cannot have multiple simultaneous active requests.[*]
    		// Until the server replies to this request, it can't read another,
    		// so we might as well run the handler in this goroutine.
    		// [*] Not strictly true: HTTP pipelining.  We could let them all process
    		// in parallel even if their responses need to be serialized.
    		serverHandler{c.server}.ServeHTTP(w, w.req)
    		
    		... // finish operation
    	}
    }
    
看一下这里面最核心的 `serverHandler{c.server}.ServeHTTP(w, w.rseq)`处理逻辑

    // serverHandler delegates to either the server's Handler or
    // DefaultServeMux and also handles "OPTIONS *" requests.
    type serverHandler struct {
    	srv *Server
    }
    
    func (sh serverHandler) ServeHTTP(rw ResponseWriter, req *Request) {
    	handler := sh.srv.Handler
    	if handler == nil {
    		handler = DefaultServeMux
    	}
    	if req.RequestURI == "*" && req.Method == "OPTIONS" {
    		handler = globalOptionsHandler{}
    	}
    	handler.ServeHTTP(rw, req)
    }
    

    // ServeHTTP dispatches the request to the handler whose
    // pattern most closely matches the request URL.
    func (mux *ServeMux) ServeHTTP(w ResponseWriter, r *Request) {
    	if r.RequestURI == "*" {
    		if r.ProtoAtLeast(1, 1) {
    			w.Header().Set("Connection", "close")
    		}
    		w.WriteHeader(StatusBadRequest)
    		return
    	}
    	h, _ := mux.Handler(r)
    	h.ServeHTTP(w, r)
    }
    
最后的`h,_:=mux.Handler`获取的是`type HandlerFunc func(ResponseWriter, *Request)`，调用ServeHTTP也就是调用了我们注册的

        http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {    
            w.Write([]byte(`hello world`))
        })
