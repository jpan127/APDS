var express = require("express");
var app = express()
//const port = 3000
var http = require("http").Server(app);
var io = require("socket.io")(http); 
var jsonfile = require('jsonfile')
var net = require('net');
 
var Log = require('log'),
    log = new Log('debug')
var spawn = require("child_process").spawn;
var port = process.env.PORT || 3000;
var PythonShell = require('python-shell');
app.use(express.static(__dirname + "/public" ));
 
app.get('/', function(req,res){
    res.redirect('index.html');
});

require('events').EventEmitter.prototype._maxListeners = 100; //max listeners


var client = new net.Socket();  
  



client.on('err', function(err){
            // handle the error safely
            console.log("Client error");
            console.log(err)
        });

io.on('connection',function(socket){ 


    //////////////////////////////////////////////////////////////////
    // Logging server stuff //
    var server = net.createServer(function(sock) {
        sock.on('err', function(err){
            // handle the error safely
            console.log(err)
        });
        sock.on('data', function(data) {
            //console.log(data);
            //Packet Type
            var packetType = data.slice(0,1).toString();
            packetType = "0x" + ascii_to_hexa(packetType);
            packetType = parseInt(packetType);
            //console.log("Packet Type: ", packetType);
            //Payload Length
            var payloadLength = data.slice(1,2).toString();
            payloadLength = "0x" + ascii_to_hexa(payloadLength);
            payloadLength = parseInt(payloadLength);
            //console.log("Payload Len: ", payloadLength);
            //Payload
            var payload = data.slice(2,payload).toString();
            //console.log(payload);
            var payload = payload.split(":");

            //console.log("Payload: ", payload[0]);
            //console.log("Payload: ", payload[1]);
            //console.log("Payload: ", payload[2]);
            console.log(packetType , payloadLength, payload);            
            
            if(packetType == 0 )
            {
                socket.broadcast.emit('infoDiag', payload);
            }
            if(packetType == 1)
            {
                socket.broadcast.emit('errorDiag', payload);
            }
            if(packetType == 2)
            {
                socket.broadcast.emit('statusDiag', payload);
            }
            if(packetType == 3)
            {
                socket.broadcast.emit('log', payload);
            }
            //var payload = data.slice(,1).toString(); 
            //var result = message.split(":");
            //console.log(result[0]);
            //console.log(result[1]);
            //console.log(result[2]);


            //byte = "0x" + ascii_to_hexa(byte);
            //byte = parseInt(byte);
            //socket.end(eval(data));
        });
    });

    server.listen(5001, '192.168.43.12'); 
    ///////////////////////////////////////////////////////////////// 


    var connected = false;
	socket.on('stream',function(image){
		socket.broadcast.emit('stream', image);
	});
    socket.on('err', function(err){
            // handle the error safely
            console.log(err)
        });
	socket.on('event',function(data){
    
        client.connect(5000, "192.168.43.250", function() { //192.168.43.250 port 5000
            console.log('Connected to 192.168.43.250'); 
            console.log('Local client sending:', data.message);
            buf = new Buffer([data.message, 0xA, 0xFF]); // 0xA is temp value
            client.write(buf);
            client.destroy();
            client = new net.Socket();
            client.on('err', function(err){
                // handle the error safely
                console.log(err)
            });
        });
	});
    socket.on('dirEvent',function(data){
        client.connect(5000, "192.168.43.250", function() { //192.168.43.250 port 5000
            console.log('Connected to 192.168.43.250'); 
            console.log('Local client sending:', data.message);
            buf = new Buffer([data.message, 0x4B, 0xFF]); // 0x4B is temp value 75
            client.write(buf);
            client.destroy();
            client = new net.Socket();
            client.on('err', function(err){
                // handle the error safely
                console.log(err)
            });
        });
    });    
    socket.on('dutyEvent',function(data){
        client.connect(5000, "192.168.43.250", function() { //192.168.43.250 port 5000
            console.log('Connected to 192.168.43.250'); 
            console.log('Local client sending duty cycle:', data.message);
            buf = new Buffer([12, data.message, 0xFF]);  
            client.write(buf);
            client.destroy();
            client = new net.Socket();
        });
    });
    socket.on('exit', function(ignore) { 
        socket.disconnect(true);
        console.log("Server disconnecting.");
        process.exit(); 
    });
    socket.on('packet', function(args) {
        connected = true;
        client.connect(5000, args.ip, function() { //192.168.43.250 port 5000
            console.log('Connected'); 
            console.log("Sending packet");
            buf = new Buffer([0x41, 0x42, 0x43, 0x44, 0x0]); // Example buffer
            client.write(buf);
            client.destroy();
            client = new net.Socket();
        });
        client.on('err', function(err){
            // handle the error safely
            console.log(err)
        });
    });      
    socket.on('packet', function(args) {
        
        console.log("Sending packet");
        //buf = new Buffer(["40"]);  
        buf = new Buffer([0x41, 0x42, 0x43, 0x44, 0x0]);
        client.write(buf);
        //client.write(args.body);
        //process.exit(); 
        client.destroy();
        client = new net.Socket();

        //TODO: Flag for connected status
        //If still connected, send another packet (TEST)
        //If not connected, reconnect and send (test?)
        //If different IP, keep first open and open another client connection
    })
 
});

io.on('err', function(err){
    // handle the error safely
    console.log(err)
})

http.listen(port,function(){
    log.info('Server listening through port %s', port);
});

http.on('err', function(err){
   // handle the error safely
    console.log(err)
});

process.on('uncaughtException', function(err) {
  console.log('Caught exception: ' + err);
});

state_machine();

function state_machine() {
    var state = ["NAVIGATING_SIDEWALK", "LOOKING_FOR_PATH" ,"FOUND_PATH_TO_HOUSE", "NAVIGATING_PATH" ,"REACHED_DOOR" , "DELIVER_PACKAGE" ]
    // Initial state Navigating Sidewalk
    var current_state = state[0];
    call_video_stream();

    setInterval(read_json_file, 500);
    read_json_file();
    
}

function call_video_stream() {
    console.log("Calling script...\n");
    //var pyshell = new PythonShell('sample.py')
}

function read_json_file(){
    //Read
var file = '../sidewalk/python/data.txt'
jsonfile.readFile(file, function(err, obj) {
    console.log(obj.angle);
  if(obj.angle > 85 && obj.angle < 95 ){
        client.connect(5000, "192.168.43.250", function() { //192.168.43.250 port 5000
            console.log('Connected to 192.168.43.250'); 
            console.log('Local client sending:', 0);
            buf = new Buffer([0, 0x4B, 0xFF]); // 0x4B is temp value 75
            client.write(buf);
            client.destroy();
            client = new net.Socket();
            client.on('err', function(err){
                // handle the error safely
                console.log(err)
            });
        });        
  } else {
        client.connect(5000, "192.168.43.250", function() { //192.168.43.250 port 5000
            console.log('Connected to 192.168.43.250'); 
            console.log('Local client sending:', 14);
            buf = new Buffer([14, 0x4B, 0xFF]); // 0x4B is temp value 75
            client.write(buf);
            client.destroy();
            client = new net.Socket();
            client.on('err', function(err){
                // handle the error safely
                console.log(err)
            });
        });        
  }


})
    //output
}

function ascii_to_hexa(str) {
    var arr1 = [];
    for (var n = 0; n < str.length; n ++) 
     {
        var hex = Number(str.charCodeAt(n)).toString(16);
        arr1.push(hex);
     }
    return arr1.join('');
} 
    