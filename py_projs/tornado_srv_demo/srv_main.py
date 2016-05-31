import tornado.ioloop
import tornado.web
import utils


class SaySomethingHandler(tornado.web.RequestHandler):
    def get(self):
        rsp_msg = "{0} hello world!\n".format(utils.get_cur_time_str())
        self.write(rsp_msg)
        print(rsp_msg)

def make_app():
    return tornado.web.Application([
        (r"/saysomething", SaySomethingHandler),
    ])

if __name__ == "__main__":
    app = make_app()
    app.listen(8808)
    tornado.ioloop.IOLoop.current().start()