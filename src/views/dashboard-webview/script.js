function getData() {
    return Math.random();
}

var layout1 = {
  title: 'CPU History',
  showlegend: true,
  legend: {"orientation": "h"},
  margin: {l: 35, r: 35, b: 35, t: 35}
}
var layout2 = {
  title: 'Memory History',
  showlegend: true,
  legend: {"orientation": "h"},
  margin: {l: 35, r: 35, b: 35, t: 35}
}
var layout3 = {
  title: 'IO History',
  showlegend: true,
  legend: {"orientation": "h"},
  margin: {l: 35, r: 35, b: 35, t: 35}
}
var layout4 = {
  title: 'Events History',
  showlegend: true,
  legend: {"orientation": "h"},
  margin: {l: 35, r: 35, b: 35, t: 35}
}

var config = {
  responsive: true,
  displayModeBar: false,
  scrollZoom: false
}

Plotly.newPlot('cpuPlot',[{
    y:[getData()],
    type:'scatter',
    mode: 'lines'
}], layout1, config);

var cnt1 = 0;

setInterval(function(){

    Plotly.extendTraces('cpuPlot',{ y:[[getData()]]}, [0]);
    cnt1++;
    if(cnt1 > 50) {
        Plotly.relayout('cpuPlot',{
            xaxis: {
                range: [cnt1-50,cnt1]
            }
        });
    }
},100);

Plotly.newPlot('memPlot',[{
    y:[getData()],
    type:'scatter',
    mode: 'lines'
}], layout2, config);

var cnt2 = 0;

setInterval(function(){

    Plotly.extendTraces('memPlot',{ y:[[getData()]]}, [0]);
    cnt2++;
    if(cnt2 > 50) {
        Plotly.relayout('memPlot',{
            xaxis: {
                range: [cnt2-50,cnt2]
            }
        });
    }
},100);

Plotly.newPlot('ioPlot',[{
    y:[getData()],
    type:'scatter',
    mode: 'lines'
}], layout3, config);

var cnt3 = 0;

setInterval(function(){

    Plotly.extendTraces('ioPlot',{ y:[[getData()]]}, [0]);
    cnt3++;
    if(cnt3 > 50) {
        Plotly.relayout('ioPlot',{
            xaxis: {
                range: [cnt3-50,cnt3]
            }
        });
    }
},100);

Plotly.newPlot('eventPlot',[{
    y:[getData()],
    type:'scatter',
    mode: 'lines'
}], layout4, config);

var cnt4 = 0;

setInterval(function(){

    Plotly.extendTraces('eventPlot',{ y:[[getData()]]}, [0]);
    cnt3++;
    if(cnt3 > 50) {
        Plotly.relayout('eventPlot',{
            xaxis: {
                range: [cnt4-50,cnt4]
            }
        });
    }
},100);

var cpuData = [

	{

		domain: { x: [0, 1], y: [0, 1] },

		value: 270,

		title: { text: "CPU" },

		type: "indicator",

		mode: "gauge+number"

	}

];


var layout5 = { margin: { t: 0, b: 0 } };

Plotly.newPlot('cpuGauge', cpuData, layout5);

var memData = [

	{

		domain: { x: [0, 1], y: [0, 1] },

		value: 270,

		title: { text: "MEM" },

		type: "indicator",

		mode: "gauge+number"

	}

];


var layout6 = { margin: { t: 0, b: 0 } };

Plotly.newPlot('memGauge', memData, layout6);

