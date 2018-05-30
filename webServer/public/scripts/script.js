var timestamps = [0,1,2,3,4,5];
var values = [0,1,2,3,4,5];

var currentSensor = null;
var timestamp = 0;
var timer = null;


var ctx = document.getElementById("myChart");
var myChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: timestamps,
    datasets: [
      {
        data: values
      }
    ]
  }
});

var socket = io.connect('http://localhost:8080');


//emit request to get the tree view
socket.emit('request', {id: 0, data: "select distinct sta,type,id from val"});

socket.on('callback', function (message) {

    //handler
    console.log(message);

    //____________UPDATE_VALUES__________________________
    if(message.id === 1){
        console.log("handler -> new table");
        changeValues(message.data, myChart);
    }

    if(message.id === 3){
        console.log("handler -> new value");
        addValues(message.data, myChart);
    }

    //___________UPDATE_TREE_____________________
    var staIndex = 0;
    var idIndex = 0;
    var typeIndex = 0

    if(message.id === 0){
        console.log("handler -> new tree");
        updateTree(message.data, $('#container'));

    }

});

function refreshValues(){
    var lastTs = myChart.data.labels[myChart.data.labels.length - 1];
    if(currentSensor){
        var request = "select timestamp,value from val where type="+currentSensor.type+" and id="+currentSensor.id+" and sta='"+currentSensor.sta+"' and timestamp>"+lastTs.toString()+";";
        socket.emit('request', {id: 3, data: request});
    }
}

function changeValues(message, chart){
    if(message){
        var timestamps = [];
        for (var i = 0; i < message.length; i++) {
            timestamps.push(message[i].timestamp);
        }

        var values = [];
        for (i = 0; i < message.length; i++) {
            values.push(message[i].value);
        }

        chart.data.labels.pop();
        chart.data.datasets.pop();

        chart.data.labels = timestamps;
        chart.data.datasets.push({data:values});

        chart.update();
    }
}

function addValues(message, chart){
    if(message){
        for (var i = 0; i<message.length; i++){
            console.log(message);
            chart.data.labels.push(message[i].timestamp);
            chart.data.datasets[0].data.push(message[i].value);
        }
        chart.update();
    }
}

function updateTree(message, jsTree){
    var tree = [];
    message.forEach(function(ele){

        var treeSta = tree.find(function(treeEle, index){
            staIndex = index;
            return treeEle.text === ele.sta;
        });
        if(treeSta){
            console.log(treeSta);
            var treeId = treeSta.children.find(function(treeEle, index){
                idIndex = index;
                return treeEle.text === ele.id.toString();
            });
            if(treeId){
                console.log(treeId);
                var treeType = treeId.children.find(function(treeEle, index){
                    typeIndex = index;
                    return treeEle.text === ele.type.toString();
                });
                if(treeType){
                    console.log(treeType);
                }
                else{
                    //create new type node
                    tree[staIndex].children[idIndex].children.push({ text: ele.type.toString()});
                }
            }
            else{
                //create new id node
                tree[staIndex].children.push({ text: ele.id.toString(), children: [
                                                     { text: ele.type.toString()}
                                               ]});
            }
        }
        else{
            //create new sta node
            tree.push({text: ele.sta, children: [
                              { text: ele.id.toString(), children: [
                                    { text: ele.type.toString()}
                              ]}
                            ]});
        }
        console.log(tree);
    });

    jsTree.jstree({
      core : {
        data : tree
      }
    });
}

//___________________JQUERY_PART___________________________________

$('#container').on('changed.jstree', function (e, data) {
    if(data.node.parents.length === 3){
        var type = data.instance.get_node(data.selected).text;
        var id = data.instance.get_node(data.node.parents[0]).text;
        var sta = data.instance.get_node(data.node.parents[1]).text;
        currentSensor = {type: type, id: id, sta: sta};

        var request = "select timestamp,value from val where type="+type+" and id="+id+" and sta='"+sta+"' and timestamp>"+timestamp.toString()+";";
        socket.emit('request', {id: 1, data: request});
    }

})

$("#sample_freq").keyup(function(e){
    if(e.keyCode === 13){
        socket.emit('command', $("#sample_freq").val());
    }
});

$("#timestamp_min").keyup(function(e){
    if(e.keyCode === 13){
        timestamp = $("#timestamp_min").val();
        var request = "select timestamp,value from val where type="+currentSensor.type+" and id="+currentSensor.id+" and sta='"+currentSensor.sta+"' and timestamp>"+timestamp.toString()+";";
        socket.emit('request', {id: 1, data: request});
    }
});

$( "#auto_refresh" ).click(function() {
    if(this.checked){
        timer = setInterval(refreshValues, 1000);
        console.log("start auto refresh");
    }
    else{
        console.log("stop auto refresh");
        clearInterval(timer);
    }
});
