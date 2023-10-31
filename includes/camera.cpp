#include <ctime>
#include <iostream>
#include <fstream>
#include <raspicam/raspicam_cv.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace std; 

void signal_callback(int signum){
    exit(signum);
}
int take_camera() {
   
    time_t timer_begin,timer_end;
    raspicam::RaspiCam_Cv camera;
    cv::Mat image;
    int nCount=1;
    //set camera params
    int width = 640;
    int height = 480;

    // Set camera parameters (optional)
    camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    camera.set(cv::CAP_PROP_FORMAT, CV_8UC3);  // RGB format
    // Set shutter speed (in microseconds)
    camera.set(cv::CAP_PROP_EXPOSURE, 700);  // Example value, adjust as needed
    // Set ISO sensitivity
    camera.set(cv::CAP_PROP_ISO_SPEED, 300);
    // Set brightness level
    camera.set(cv::CAP_PROP_BRIGHTNESS, 60); 
   
    //Open camera
    cout<<"Opening Camera..."<<endl;
    if (!camera.open()) {cerr<<"Error opening the camera"<<endl;return -1;}
    //Start capture
    cout<<"Capturing "<<nCount<<" frames ...."<<endl;
    time ( &timer_begin );
    int key = cv::waitKey(3000);
    camera.grab();
    camera.retrieve ( image);
        // if ( i%5==0 )  cout<<"\r captured "<<i<<" images"<<std::flush;
    
    cout<<"\nStop camera..."<<endl;
   
    //show time statistics
    time ( &timer_end ); /* get current time; same as: timer = time(NULL)  */
    double secondsElapsed = difftime ( timer_end,timer_begin );
    cout<< secondsElapsed<<" seconds for "<< nCount<<"  frames : FPS = "<<  ( float ) ( ( float ) ( nCount ) /secondsElapsed ) <<endl;
    //save image 
 
    cv::imwrite("../output/image3.jpg", image, {cv::IMWRITE_JPEG_QUALITY, 60});

    cout<<"Image saved at raspicam_cv_image.jpg"<<endl;
    camera.release();
    return 0;
}

int takeVideo() { //success
    // Initialize camera and video writer
    raspicam::RaspiCam_Cv camera;
    cv::VideoWriter videoWriter;
    signal(SIGINT, signal_callback);
    
    // Initialize time measurement
    std::chrono::duration<double> duration;
    std::chrono::milliseconds sleepDuration(20); //20 ms
    auto start = std::chrono::high_resolution_clock::now();

    int width = 640;
    int height = 480;
    int fps = 15;  // Frames per second
    int durasi_video = 4;

    // Set camera parameters (optional)
    camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    camera.set(cv::CAP_PROP_FORMAT, CV_8UC3);  // RGB format
    // Set shutter speed (in microseconds)
    camera.set(cv::CAP_PROP_EXPOSURE, 1000);  // Example value, adjust as needed
    // Set ISO sensitivity
    camera.set(cv::CAP_PROP_ISO_SPEED, 200);
    // Set brightness level
    camera.set(cv::CAP_PROP_BRIGHTNESS, 60);  // Example value (0 to 100), adjust as needed
    if (!camera.open()) {
        std::cerr << "Error: Couldn't open the camera." << std::endl;
        return -1;
    }
    int key = cv::waitKey(1000);
    std::this_thread::sleep_for(sleepDuration);

    // Define the codec and create a VideoWriter object
    int fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');  // H264 codec
    
    cv::Size frameSize(camera.get(cv::CAP_PROP_FRAME_WIDTH), camera.get(cv::CAP_PROP_FRAME_HEIGHT));
    videoWriter.open("output_video.mp4", fourcc, fps, frameSize, true);

    // Check if the VideoWriter opened successfully
    if (!videoWriter.isOpened()) {
        std::cerr << "Error: Couldn't open the VideoWriter." << std::endl;
        return -1;
    }

    int index = 0;
    int frame_no = 1;
    auto end = std::chrono::high_resolution_clock::now();
      // Calculate the duration between start and end
    duration = end - start;
    std::cout << "Elapsed time: " << duration.count() << " seconds" << std::endl;
    
    auto beginning = std::chrono::high_resolution_clock::now();
    char buffer[100]; // Adjust the size as needed
    const char* newFileName;
    const char* oldFileName = "output_video.mp4";
    // Capture and save video frames
    while (true) {
        start = std::chrono::high_resolution_clock::now();
        cv::Mat frame;
        camera.grab();
        camera.retrieve(frame);  // Capture frame from the camera

        // Write the frame to the video file
        videoWriter.write(frame);
       
        if (cv::waitKey(50) >= 0) {
            std::cout << "27 pressed" << std::endl;
            break;
        }
        //std::this_thread::sleep_for(sleepDuration);

        index = index + 1;
        std::cout << "Frame Number: " << index << std::endl;

        if (index == (durasi_video*fps*frame_no)) {
            //break;
            //Rename FILE =====
            videoWriter.release();
            int charsWritten = std::sprintf(buffer, "%dfps-%ds-%.2fs-%dp-%d.mp4", fps, durasi_video,duration,height,frame_no);
            newFileName = buffer;
            if (std::rename(oldFileName, newFileName) != 0) {
                std::perror("Error renaming file");
                return -1;
            }
            frame_no=frame_no+1;
            
            videoWriter.open("output_video.mp4", fourcc, fps, frameSize, true);
            //Rename FILE =====
        }
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        std::cout << "Elapsed time: " << duration.count() << " seconds" << std::endl;
    }
    end = std::chrono::high_resolution_clock::now();
    duration = end - beginning;
    std::cout << "Total time: " << duration.count() << " seconds" << std::endl;
    // Release resources
    camera.release();
    videoWriter.release();
    end = std::chrono::high_resolution_clock::now();
    duration = end - beginning;
    std::cout << "Release time: " << duration.count() << " seconds" << std::endl;
    std::cout << "Fps: " << fps << " seconds" << std::endl;
    std::cout << "Durasi_video: " << durasi_video << " seconds" << std::endl;

    //Rename FILE =====
    // const char* oldFileName = "output_video.mp4";
    // char buffer[100]; // Adjust the size as needed
    // int charsWritten = std::sprintf(buffer, "%dfps-%ds-%.2fs-%dp.mp4", fps, durasi_video,duration,height);
    // const char* newFileName = buffer;
    // if (std::rename(oldFileName, newFileName) != 0) {
    //     std::perror("Error renaming file");
    //     return -1;
    // }
    //Rename FILE =====
    return 0;
}

int readVideoFrames() {
    // Open the video file
    std::cout << "Read Video file" << std::endl;
    cv::VideoCapture cap("output_video.mp4");

    // Check if the video file opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Error: Couldn't open the video file." << std::endl;
        return -1;
    }

    // Get video properties
    int fps = static_cast<int>(cap.get(cv::CAP_PROP_FPS));
    int frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    cv::Size frameSize(static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH)),
                       static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)));

    // Create a vector to store video frames
    std::vector<cv::Mat> videoFrames;

    // Read video frames and store them in the vector
    cv::Mat frame;
    while (cap.read(frame)) {
        videoFrames.push_back(frame.clone());

        // You can perform additional processing on 'frame' if needed

        // Break the loop if you want to process a specific number of frames
        // or use any other condition to stop reading frames.
    }

    // Release resources
    cap.release();
    size_t totalBytes = 0;
    size_t numberOfFrames = videoFrames.size();

    std::cout << "Number of frames in the vector: " << numberOfFrames << std::endl;
    for (const auto& frame : videoFrames) {
        totalBytes += frame.total() * frame.elemSize();
    }

    std::cout << "Total number of bytes for all frames: " << totalBytes << " bytes." << std::endl;

    // Now, the video frames are stored in the vector 'videoFrames'
    // You can access and process each frame as needed.

    return 0;
}

int readVideointoBuffer(){

    const std::string filePath = "output_video.mp4";

    // Open the file
    std::ifstream file(filePath, std::ios::binary);

    // Check if the file is open
    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open the file." << std::endl;
        return -1;
    }

    // Get the size of the file
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create a buffer to store the file contents
    std::vector<char> buffer(fileSize);

    // Read the file contents into the buffer
    file.read(buffer.data(), fileSize);

    // Close the file
    file.close();
    std::cout << "Buffer Length: " << buffer.size() << " elements" << std::endl;
    std::cout << "Buffer Size: " << buffer.size() * sizeof(char) << " bytes" << std::endl;
    // Now, the file contents are stored in the 'buffer'
    // You can access and process the data as needed.

    // Example: Print the contents of the buffer
    // for (char c : buffer) {
    //     std::cout << c;
    // }

    return 0;
}