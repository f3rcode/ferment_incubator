import grafica.*;
import processing.serial.*;
import de.bezier.guido.*;
 
Serial port;  // Create object from Serial class
GPlot plot; //<>//
GPointsArray pointsTemp;
GPointsArray pointsOut;
GPoint point= new GPoint(0,0);
int nPoints = 100;

int val_T, val_V;      // Data received from the serial port
int[] values_V;
int[] values_T;
float zoom;
float scale = 5;

int maxLength =2;     //Lenght of incomingValues
float[] incomingValues=new float[2]; //On the AVR Arduinos such as Arduino Uno, double is the same thing as float and has 32bit (4B)

void setup()
{ 
  size(700, 350);
 
  println(Serial.list());  // List all the available serial ports
  String portName = Serial.list()[0]; //we chose the ttyACM0, the 1st on the list once FLORA is connected
  port = new Serial(this, portName, 9600);
  port.clear();
  // Prepare the points for the plot
  pointsTemp = new GPointsArray(nPoints);
   pointsOut = new GPointsArray(nPoints);
  
  for (int i = 0; i < nPoints; i++) {
    pointsTemp.add(i, 0);
    pointsOut.add(i, 0);
  }

  // Create a new plot and set its position on the screen
  plot = new GPlot(this);
  //plot.setPos(25, 25);
  // or all in one go
  // GPlot plot = new GPlot(this, 25, 25);
  plot.setDim(700, 300);

  // Set the plot title and the axis labels
  plot.setTitleText("Temperature");
  plot.getXAxis().setAxisLabelText("time");
  plot.getYAxis().setAxisLabelText("temp");

  // Add the points //<>//
  plot.setPoints(pointsTemp); 
  plot.addLayer("output", pointsOut);

 // Change the second layer line color
  plot.getLayer("output").setLineColor(color(100, 255, 100));
  
  // Activate the panning effect
  plot.activatePanning();
 
  delay(1000);
}
 
void serialEvent (Serial myPort) {
  
  try {
     String inString;
     if ((inString=myPort.readStringUntil('\n')) == null) return;
    
    inString=trim(inString);
    String inStringArray[]=splitTokens(inString, ": ");
    println(inString);
    
    for (int i=0; i<maxLength; i++){
    incomingValues[i]=float(inStringArray[i]);
    }

  }
  catch(RuntimeException e) {
    e.printStackTrace();
  }
}


void draw()
{
  
    background(150);

    plot.beginDraw();
    plot.drawBackground();
    plot.drawBox();
    plot.drawXAxis();
    plot.drawYAxis();
    plot.drawTopAxis();
    plot.drawRightAxis();
    plot.drawTitle();
    plot.setPoints(pointsTemp); 
    plot.getLayer("output").setPoints(pointsOut);
    plot.setLineColor(color(100, 100, 100));

    plot.getMainLayer().drawPoints();
    plot.getLayer("output").drawPoints();
    plot.drawGridLines(GPlot.VERTICAL);
    //plot.drawFilledContours(GPlot.HORIZONTAL, 28);
    plot.setPointColor(color(0, 200, 0));
    plot.drawLegend(new String[] {Float.toString(incomingValues[0]), Float.toString(incomingValues[1])}, new float[] {0.07, 0.22}, 
                  new float[] {0.92, 0.92});
    plot.drawLabels();
    plot.endDraw();
  
    printArray(incomingValues); //<>//
   
   // Add the point at the end of the array
   float now=millis();
  
   pointsTemp.add(new GPoint(now, incomingValues[0]));//PARAMETER=ONE OF THE SERIAL INPUT VALUES (LETS BEGIN WITH TEMPERATURE AND OUTPUT WILL BE THE NEXT ONE
   pointsOut.add(new GPoint(now, incomingValues[1]));

   // Remove the last point
   pointsTemp.remove(0);
   pointsOut.remove(0);

      
   delay(500);
 
}
