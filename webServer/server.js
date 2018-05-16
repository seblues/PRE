var express = require('express');

var app = express();
var spawn  = require('child_process').spawn;
var child = spawn('../zbServer/build/zigbeeServer');

child.stdout.on('data', function(data){
  console.log(data.toString());
});


//create server listening on port 8080
var server = require('http').createServer(app);
var io = require('socket.io').listen(server);
server.listen(8080);

app.use(express.static('public'));

var sqlite3 = require('sqlite3').verbose();  
var file = "prgm.db";
var db = new sqlite3.Database(file);  

io.on('connection', function(socket){
    socket.on('request', function(sqlRequest){
        console.log(sqlRequest);

        db.all(sqlRequest.data, [], function(err, rows){
                   var string = JSON.stringify(rows);
                   socket.emit('callback', {id: sqlRequest.id, data: rows});
        });
    });

    socket.on('command', function(command){
        console.log(command);
        child.stdin.write(command.toString() + '\n');
    });
});


  
