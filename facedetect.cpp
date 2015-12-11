#include "opencv2/objdetect.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/utility.hpp"

#include "opencv2/videoio/videoio_c.h"
#include "opencv2/highgui/highgui_c.h"

#include <cctype>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


using namespace std;
using namespace cv;

class tcp_client{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;
     
public:
    tcp_client();
    bool conn(string, int);
    bool send_data(string data);
};

void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip, tcp_client c);

string cascadeName = "../../data/haarcascades/haarcascade_frontalface_alt.xml";
string nestedCascadeName = "../../data/haarcascades/haarcascade_eye_tree_eyeglasses.xml";



tcp_client::tcp_client(){
    sock = -1;
    port = 0;
    address = "";
}

bool tcp_client::conn(string address , int port){
    //create socket if it is not already created
    if(sock == -1)
    {
        //Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            perror("Could not create socket");
        }
         
        cout<<"Socket created\n";
    }
    else    {   /* OK , nothing */  }
     
    //setup address structure
    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *he;
        struct in_addr **addr_list;
         
        //resolve the hostname, its not an ip address
        if ( (he = gethostbyname( address.c_str() ) ) == NULL)
        {
            //gethostbyname failed
            herror("gethostbyname");
            cout<<"Failed to resolve hostname\n";
             
            return false;
        }
         
        //Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
        addr_list = (struct in_addr **) he->h_addr_list;
 
        for(int i = 0; addr_list[i] != NULL; i++)
        {
            //strcpy(ip , inet_ntoa(*addr_list[i]) );
            server.sin_addr = *addr_list[i];
             
            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;
             
            break;
        }
    }
     
    //plain ip address
    else
    {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }
     
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
     
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    cout<<"Connected\n";
    return true;
}

bool tcp_client::send_data(string data){
    //Send some data
    if( send(sock , data.c_str() , strlen( data.c_str() ) , 0) < 0)
    {
        perror("Send failed : ");
        return false;
    }
    return true;
}



int main( int argc, const char** argv ){

    /*
    SET UP CLIENT
    */

    tcp_client myClient;
    string host;
    int port;

    cout << "Enter the host to connect to \n";
    cin >> host; 

    cout << "Enter the port number to connect to \n";
    cin >> port;

    myClient.conn(host, port);

    

    CvCapture* capture = 0;
    Mat frame, frameCopy, image;
    
    const string scaleOpt = "--scale=";
    size_t scaleOptLen = scaleOpt.length();
    
    const string cascadeOpt = "--cascade=";
    size_t cascadeOptLen = cascadeOpt.length();
    
    const string nestedCascadeOpt = "--nested-cascade";
    size_t nestedCascadeOptLen = nestedCascadeOpt.length();
    
    const string tryFlipOpt = "--try-flip";
    size_t tryFlipOptLen = tryFlipOpt.length();
    
    string inputName;
    bool tryflip = false;

    

    CascadeClassifier cascade, nestedCascade;
    double scale = 1;

    for( int i = 1; i < argc; i++ )
    {
        cout << "Processing " << i << " " <<  argv[i] << endl;
        if( cascadeOpt.compare( 0, cascadeOptLen, argv[i], cascadeOptLen ) == 0 )
        {
            cascadeName.assign( argv[i] + cascadeOptLen );
            cout << "  from which we have cascadeName= " << cascadeName << endl;
        }
        else if( nestedCascadeOpt.compare( 0, nestedCascadeOptLen, argv[i], nestedCascadeOptLen ) == 0 )
        {
            if( argv[i][nestedCascadeOpt.length()] == '=' )
                nestedCascadeName.assign( argv[i] + nestedCascadeOpt.length() + 1 );
            if( !nestedCascade.load( nestedCascadeName ) )
                cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
        }
        else if( scaleOpt.compare( 0, scaleOptLen, argv[i], scaleOptLen ) == 0 )
        {
            if( !sscanf( argv[i] + scaleOpt.length(), "%lf", &scale ) || scale < 1 )
                scale = 1;
            cout << " from which we read scale = " << scale << endl;
        }
        else if( tryFlipOpt.compare( 0, tryFlipOptLen, argv[i], tryFlipOptLen ) == 0 )
        {
            tryflip = true;
            cout << " will try to flip image horizontally to detect assymetric objects\n";
        }
        else if( argv[i][0] == '-' )
        {
            cerr << "WARNING: Unknown option %s" << argv[i] << endl;
        }
        else
            inputName.assign( argv[i] );
    }

    if( !cascade.load( cascadeName ) ){
        cerr << "ERROR: Could not load classifier cascade" << endl;
        return -1;
    }

    if( inputName.empty() || (isdigit(inputName.c_str()[0]) && inputName.c_str()[1] == '\0') ){
        capture = cvCaptureFromCAM( inputName.empty() ? 0 : inputName.c_str()[0] - '0' );
        int c = inputName.empty() ? 0 : inputName.c_str()[0] - '0' ;
        if(!capture) cout << "Capture from CAM " <<  c << " didn't work" << endl;
    }
    else if( inputName.size() ){
        image = imread( inputName, 1 );
        if( image.empty() ){
            capture = cvCaptureFromAVI( inputName.c_str() );
            if(!capture) cout << "Capture from AVI didn't work" << endl;
        }
    }
    else{
        image = imread( "../data/lena.jpg", 1 );
        if(image.empty()) cout << "Couldn't read ../data/lena.jpg" << endl;
    }

    cvNamedWindow( "result", WINDOW_NORMAL );

    if( capture ){
        cout << "In capture ..." << endl;
        for(;;)
        {
            IplImage* iplImg = cvQueryFrame( capture );
            frame = cv::cvarrToMat(iplImg);
            if( frame.empty() )
                break;
            if( iplImg->origin == IPL_ORIGIN_TL )
                frame.copyTo( frameCopy );
            else
                flip( frame, frameCopy, 0 );

            

            detectAndDraw( frameCopy, cascade, nestedCascade, scale, tryflip, myClient );

            if( waitKey( 10 ) >= 0 )
                goto _cleanup_;
        }

        waitKey(0);

_cleanup_:
        cvReleaseCapture( &capture );
    }
    else
    {
        cout << "In image read" << endl;
        if( !image.empty() )
        {
            detectAndDraw( image, cascade, nestedCascade, scale, tryflip, myClient );
            waitKey(0);
        }
        else if( !inputName.empty() )
        {
            /* assume it is a text file containing the
            list of the image filenames to be processed - one per line */
            FILE* f = fopen( inputName.c_str(), "rt" );
            if( f )
            {
                char buf[1000+1];
                while( fgets( buf, 1000, f ) )
                {
                    int len = (int)strlen(buf), c;
                    while( len > 0 && isspace(buf[len-1]) )
                        len--;
                    buf[len] = '\0';
                    cout << "file " << buf << endl;
                    image = imread( buf, 1 );
                    if( !image.empty() )
                    {
                        detectAndDraw( image, cascade, nestedCascade, scale, tryflip, myClient );
                        c = waitKey(0);
                        if( c == 27 || c == 'q' || c == 'Q' )
                            break;
                    }
                    else
                    {
                        cerr << "Aw snap, couldn't read image " << buf << endl;
                    }
                }
                fclose(f);
            }
        }
    }

    cvDestroyWindow("result");

    return 0;
}



void detectAndDraw( Mat& img, CascadeClassifier& cascade, CascadeClassifier& nestedCascade, double scale, bool tryflip, tcp_client c ){
   

    int i = 0;
    double t = 0;
    vector<Rect> faces, faces2;
    const static Scalar colors[] =  { CV_RGB(255,255,255),
        CV_RGB(0,128,255),
        CV_RGB(0,255,255),
        CV_RGB(0,255,0),
        CV_RGB(255,128,0),
        CV_RGB(255,255,0),
        CV_RGB(255,0,0),
        CV_RGB(255,0,255)} ;
    Mat gray, smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1 );

    cvtColor( img, gray, COLOR_BGR2GRAY );
    resize( gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR );
    equalizeHist( smallImg, smallImg );

    t = (double)cvGetTickCount();
    cascade.detectMultiScale( smallImg, faces,
        1.1, 2, 0
        |CASCADE_FIND_BIGGEST_OBJECT
        //|CASCADE_DO_ROUGH_SEARCH
        //|CASCADE_SCALE_IMAGE
        ,
        Size(30, 30) ); //Default 30,30
    if( tryflip )
    {
        flip(smallImg, smallImg, 1);
        cascade.detectMultiScale( smallImg, faces2,
                                 1.1, 2, 0
                                 //|CASCADE_FIND_BIGGEST_OBJECT
                                 //|CASCADE_DO_ROUGH_SEARCH
                                 //|CASCADE_SCALE_IMAGE
                                 ,
                                 Size(30, 30) );

        for( vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); r++ ){
            faces.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));

        }
    }

  
    for( vector<Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++ ){
        string to_send = "";
        Point biggest;

        Mat smallImgROI;
        vector<Rect> nestedObjects;
        Point center;
        Scalar color = colors[i%8];
        int radius;
	
	    


        double aspect_ratio = (double)r->width/r->height;
        if( 0.75 < aspect_ratio && aspect_ratio < 1.3 ){

            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            if(radius > 150){
                // Green circle for the middle of the face
                circle(img, center, 10, Scalar(0,255,0), -1, 8);
                biggest = center;
            }
           cout << biggest.x << "   " << biggest.y << endl;
         
           
        circle( img, center, radius, color, 3, 8, 0 );



        Point middle = Point(640, 360);

        Point left = Point(250, 360);
        Point right = Point(430, 360);
        Point top = Point(340, 435);
        Point bottom = Point(340, 285);
        //Put your face here
        circle(img, middle, 200, Scalar(0,255,0), 5, 8);    
    

        //Left
        Point pleft1 = Point(530, 290);
        Point pleft2 = Point(600, 430);
        rectangle(img, pleft1, pleft2, Scalar(255,255,0), 3, 8);
        
        //Right
        Point pright1 = Point(680, 290);
        Point pright2 = Point(750, 430);
        rectangle(img, pright1, pright2, Scalar(0,255,255), 3, 8);

        //Bottom
        Point ptop1 = Point(530, 390);
        Point ptop2 = Point(750, 430);
        rectangle(img, ptop1, ptop2, Scalar(255,0,255), 3, 8);

        //Top
        Point pbot1 = Point(530, 290);
        Point pbot2 = Point(750, 330);
        rectangle(img, pbot1, pbot2, Scalar(255,0,0), 3, 8);

        if(biggest.x < 600 && biggest.x > 530 && biggest.y > 330 && biggest.y < 390){
            cout << "LEFT" << endl;
            to_send = "1";
        }

        if(biggest.x < 750 && biggest.x > 680 && biggest.y > 330 && biggest.y < 390){
            cout << "RIGHT" << endl;
            to_send = "2";
        }

        if(biggest.x < 680 && biggest.x > 600 && biggest.y > 390 && biggest.y < 430){
            cout << "DOWN" << endl;
            to_send = "4";
        }

        if(biggest.x < 680 && biggest.x > 600 && biggest.y > 290 && biggest.y < 330){
            cout << "UP" << endl;
            to_send = "3";
        }

        //Upper-Left
         if(biggest.x < 600 && biggest.x > 530 && biggest.y > 290 && biggest.y < 330){
            cout << "UP-LEFT" << endl;
            to_send = "10";
        }       

        //Upper-Right
        if(biggest.x < 750 && biggest.x > 680 && biggest.y > 290 && biggest.y < 330){
            cout << "UP-Right" << endl;
            to_send = "11";
        }

        //Down-Left
        if(biggest.x < 600 && biggest.x > 530 && biggest.y > 390 && biggest.y < 430){
            cout << "Down-Left" << endl;
            to_send = "12";
        }

        // Down-Right
        if(biggest.x < 750 && biggest.x > 680 && biggest.y > 390 && biggest.y < 430){
            cout << "Down-Right" << endl;
            to_send = "13";
        }

    
        c.send_data(to_send);

        }
	
        else
            rectangle( img, cvPoint(cvRound(r->x*scale), cvRound(r->y*scale)),
                       cvPoint(cvRound((r->x + r->width-1)*scale), cvRound((r->y + r->height-1)*scale)),
                       color, 3, 8, 0);
        if( nestedCascade.empty() )
            continue;
        smallImgROI = smallImg(*r);
        nestedCascade.detectMultiScale( smallImgROI, nestedObjects,
            1.1, 2, 0
            |CASCADE_FIND_BIGGEST_OBJECT
            //|CASCADE_DO_ROUGH_SEARCH
            //|CASCADE_DO_CANNY_PRUNING
            //|CASCADE_SCALE_IMAGE
            ,
            Size(60, 60) );
	 
        for( vector<Rect>::const_iterator nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++ )
        {
            center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
            center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
            radius = cvRound((nr->width + nr->height)*0.25*scale);

            circle( img, center, radius, color, 3, 8, 0 );
	    
        }

    }

    cv::imshow( "result", img ); 
}
