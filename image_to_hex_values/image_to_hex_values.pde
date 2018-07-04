/**
 *
 */

PImage img;
PrintWriter Output;
int cont=0;
char freq=0;
void setup() {
size(320, 256);
img=loadImage("test.png");
Output=createWriter("transmitnew.hex");
loadPixels();
background(img);
 stroke(#00FF01);
for(int lineSet=97;lineSet<=128;lineSet++)
{
  for(int pixelCount=0;pixelCount<320;pixelCount++)
  {
    freq=getfreqavg((((lineSet*2)-1)+pixelCount),(((lineSet*2)-1)+pixelCount),1);
    line(pixelCount ,(lineSet*2)-1, 320 , (lineSet*2)-1);
    println((int)freq);
    Outprint(freq);
   // Output.flush();
  }
  for(int pixelCount=0;pixelCount<320;pixelCount++)
  {
    freq=getfreqavg((((lineSet*2)-1)+pixelCount),((lineSet*2)+pixelCount),2);
    line(pixelCount ,(lineSet*2)-1, 320 , (lineSet*2)-1);
    println((int)freq);
    Outprint(freq);
   // Output.flush();
  }
  for(int pixelCount=0;pixelCount<320;pixelCount++)
  {
    freq=getfreqavg((((lineSet*2)-1)+pixelCount),((lineSet*2)+pixelCount),3);
    line(pixelCount ,(lineSet*2)-1, 320 , (lineSet*2)-1);
    println((int)freq);
    Outprint(freq);
   // Output.flush();
  }
  for(int pixelCount=0;pixelCount<320;pixelCount++)
  {
    freq=getfreqavg((((lineSet*2))+pixelCount),(((lineSet*2))+pixelCount),1);
    line(pixelCount ,(lineSet*2)-1, 320 , (lineSet*2)-1);
    println((int)freq);
    Outprint(freq);
   // Output.flush();
  }    
}
}
char getfreqavg(int loc1,int loc2, int type)
{
float freq=0;
float r1= red(img.pixels[loc1]);
float g1= green(img.pixels[loc1]);
float b1= blue(img.pixels[loc1]);
float r2= red(img.pixels[loc2]);
float g2= green(img.pixels[loc2]);
float b2= blue(img.pixels[loc2]);
switch(type)
{
  case 1 : float Y1 = 16.0 + (.003906 * ((65.738 * r1) + (129.057 * g1) + (25.064 *b1)));   freq+=  1500 + (Y1 * 3.1372549); float Y2 = 16.0 + (.003906 * ((65.738 * r2) + (129.057 * g2) + (25.064 *b2)));   freq+=  1500 + (Y2 * 3.1372549); break;
  case 2 : float RY1 = 128.0 + (.003906 * ((112.439 * r1) + (-94.154 * g1) + (-18.285 * b1)));  freq+=   1500 + (RY1 * 3.1372549); float RY2 = 128.0 + (.003906 * ((112.439 * r2) + (-94.154 * g2) + (-18.285 * b2)));  freq+=   1500 + (RY2 * 3.1372549); break;
  case 3 : float BY1 = 128.0 + (.003906 * ((-37.945 * r1) + (-74.494 * g1) + (112.439 * b1)));   freq+=  1500 + (BY1 * 3.1372549); float BY2 = 128.0 + (.003906 * ((-37.945 * r2) + (-74.494 * g2) + (112.439 * b2)));   freq+=  1500 + (BY2 * 3.1372549); break;
}
freq/=2;
return (char)freq;
}

void draw() { 

}
void Outprint(char data)
{
 //Output.print(data);
 byte out=0;
 out = (byte)((data>>8) & 0x00FF);
 Output.print(hex(out));
 out = (byte)(data & 0x00FF);
 Output.print(hex(out));
}

void keyPressed()
{
Output.flush();
Output.close();
exit();
}
