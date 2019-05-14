const char homePage[] PROGMEM = R"(<!DOCTYPE html>
<html>
    <head>
        <title>Home page</title>
    </head>
<body>
    <center>
        <h2> Welcome to Home page</h2>
    </center>
    <br/>
    <a style="text-decoration:none" align="left" href="/controlpage"> Go to control page </a>
    <a style="text-decoration:none" align="right" href="/dashboardpage"> Go to dashboard page </a>

</body>
</html>)";

const char controlPage[] PROGMEM = R""(<!DOCTYPE html>
<html>
    <head>
        <title>Control Page</title>
    </head>
    <body>
        <a href="/"> back </a>
        <center>
            <h2> Welcome to control page</h2>
        </center>
        <form name="control_led_form">
            <input type="submit" value="On" onclick="javascript: form.action='/ledOn';">
            <input type="submit" value="Off" onclick="javascript: form.action='/ledOff';">
        </form>
        <div>
            <h3 id="led_status"> LED is {ledStatus} </h3>
        </div>
    </body>
</html>
)"";

const char dashboardPage[] PROGMEM = R""(<!DOCTYPE html>
<html>
    <head>
        <title>Dashboard Page</title>
        <!-- Resources -->
        <script src="https://www.amcharts.com/lib/4/core.js"></script>
        <script src="https://www.amcharts.com/lib/4/charts.js"></script>
        <script src="https://www.amcharts.com/lib/4/themes/animated.js"></script>
    </head>
    <body>
        <a href="/"> back </a>
        <center>
            <h2> Welcome to dashboard page</h2>
        </center>
        <h4>hall sensor is <span id="hall_val">0</span></h4>
        <div class="gauge_section">
            <center>
                <h2>for this section you need internet connection</h2>
            </center>
            <div id="hall_chart" style="width: 100%; height: 300px"></div>
        </div>
        
        <script>
        /*
        setInterval(function(){
        var httpRequest = new XMLHttpRequest();
        httpRequest.onreadystatechange = function(){
            if(this.readyState == 4 && this.status == 200){
                document.getElementById("hall_val").innerHTML = JSON.parse(this.responseText)['hall_val'];
            }
            return;
        };
        httpRequest.open("GET", "/hallSensor", true);
        httpRequest.send();
        },5000)
        */
       
        am4core.ready(function() {

        // Themes begin
        am4core.useTheme(am4themes_animated);
        // Themes end

        // create chart
        var chart = am4core.create("hall_chart", am4charts.GaugeChart);
        chart.innerRadius = am4core.percent(82);

        /**
         * Normal axis
         */

        var axis = chart.xAxes.push(new am4charts.ValueAxis());
        axis.min = -40;
        axis.max = 40;
        axis.strictMinMax = true;
        axis.renderer.radius = am4core.percent(80);
        axis.renderer.inside = true;
        axis.renderer.line.strokeOpacity = 1;
        axis.renderer.ticks.template.strokeOpacity = 1;
        axis.renderer.ticks.template.length = 10;
        axis.renderer.grid.template.disabled = true;
        axis.renderer.labels.template.radius = 40;

        /**
         * Axis for ranges
         */

        var colorSet = new am4core.ColorSet();

        var axis2 = chart.xAxes.push(new am4charts.ValueAxis());
        axis2.min = -40;
        axis2.max = 40;
        axis2.renderer.innerRadius = 10
        axis2.strictMinMax = true;
        axis2.renderer.labels.template.disabled = true;
        axis2.renderer.ticks.template.disabled = true;
        axis2.renderer.grid.template.disabled = true;

        var range0 = axis2.axisRanges.create();
        range0.value = -40;
        range0.endValue = 0;
        range0.axisFill.fillOpacity = 1;
        range0.axisFill.fill = colorSet.getIndex(0);

        var range1 = axis2.axisRanges.create();
        range1.value = 0;
        range1.endValue = 40;
        range1.axisFill.fillOpacity = 1;
        range1.axisFill.fill = colorSet.getIndex(2);

        /**
         * Label
         */

        var label = chart.radarContainer.createChild(am4core.Label);
        label.isMeasured = false;
        label.fontSize = 38;
        label.x = am4core.percent(50);
        label.y = am4core.percent(100);
        label.horizontalCenter = "middle";
        label.verticalCenter = "bottom";
        label.text = "0";


        /**
         * Hand
         */

        var hand = chart.hands.push(new am4charts.ClockHand());
        hand.axis = axis2;
        hand.innerRadius = am4core.percent(20);
        hand.startWidth = 10;
        hand.pin.disabled = true;
        hand.value = 0;

        hand.events.on("propertychanged", function(ev) {
        range0.endValue = ev.target.value;
        range1.value = ev.target.value;
        axis2.invalidate();
        });

        setInterval(() => {
            var httpRequest = new XMLHttpRequest();
            httpRequest.onreadystatechange = function(){
                if(this.readyState == 4 && this.status == 200){
                    var hall_val = JSON.parse(this.responseText)['hall_val']
                    document.getElementById("hall_val").innerHTML = hall_val;

                    label.text = hall_val;
                    var animation = new am4core.Animation(hand, {
                        property: "value",
                        to: hall_val
                    }, 1000, am4core.ease.cubicOut).start();
                }
                return;
            };
            httpRequest.open("GET", "/hallSensor", true);
            httpRequest.send();
        },5000);

        }); // end am4core.ready()

        </script>
    </body>
</html>
)"";