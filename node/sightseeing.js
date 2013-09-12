var http = require('http');
var net = require('net');

// send to Browser
var server = http.createServer(function(in_req, in_res){
    in_res.writeHeader(200, {'Content-Type': 'text/html'});
    in_res.writeBody('<h1>Hello world</h1>');
    in_res.finish();
});
server.listen(1337);

var io = require('socket.io').listen(server);
io.sockets.on('connection', function (socket) {
    socket.emit('info', { msg: 'welcome' });
    socket.on('msg', function (msg) { io.sockets.emit('msg', {msg: msg}); });
    socket.on('disconnect', function(){ socket.emit('info', {msg: 'bye'}); });
});

net.createServer(function(in_socket){
    in_socket.on('data', function (msg) { 
        msg_str = msg.toString();
        console.log(msg_str);
        io.sockets.emit('msg', {msg: msg_str}); 
    });
}).listen(8821, "127.0.0.1");

