# QPusher
`QPusher` is a pusher client written in c++ for a Qt application.
currenly only supporting basic functionality. the supported functions is:
* subscribe and receive events
* external authentication 
* support for secure connection (support for ws and wss. you should configure openssl correctly to be abble to use wss connection)

## Examples
you can see the `examples` folder for see how using the library and i will add more exapmles in the future


## how to use
you can simple copy the `src` folder to your project and using the `QPusherClient` class to connect to the pusher servers or you can add `QPusher.pro` file to the library in Qt creator.

**the library is needed `websocket` and `network` modules from Qt so don't forget to add those modules in your build system**

**currently the repository is under development and i will be happy to receive your bug report**


## TODO
* support for `PrivateEncrypted` channels
* automatic reconnect process

at the end huge thanks to **Pysher** repo that helps alot to understanding of the pusher servers.
