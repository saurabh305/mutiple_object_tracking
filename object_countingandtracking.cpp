// object_tracking_using_opencv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include"stdafx.h"
#include<iostream>
#include<opencv2\opencv.hpp>

using namespace std;
using namespace cv;

VideoCapture cap;

class ball
{
public:
	//~ball();

	vector<Point> contour;
	vector<Point> center_of_mass;
	Rect bounding_box;
	Point expected_next_position;

	bool flag;
	bool object_been_tracked;

	int NumOfConsecutiveFramesWithoutAMatch;

	ball(vector<Point> _contour)
	{
		contour = _contour;
		Moments mu = moments(contour);
		bounding_box = boundingRect(contour);

		Point temp;
		temp.x = int(mu.m10 / mu.m00);
		temp.y = int(mu.m01 / mu.m00);

		center_of_mass.push_back(temp);

		flag = true;
		object_been_tracked = true;

		NumOfConsecutiveFramesWithoutAMatch = 0;

	}

	void next_position()
	{

		int numPositions = (int)center_of_mass.size();

		if (numPositions == 1) {

			expected_next_position.x = center_of_mass.back().x;
			expected_next_position.y = center_of_mass.back().y;

		}

		else if (numPositions >= 2)
		{
			Point position1, position2;
			for (int i = numPositions - 1; i >= numPositions - 2; i--)
			{
				if (i == center_of_mass.size()-2)
				{
					position1.x = center_of_mass[i].x;
					position1.y = center_of_mass[i].y;
				}
				if (i == (center_of_mass.size() - 1))
				{
					position2.x = center_of_mass[i].x;
					position2.y = center_of_mass[i].y;
				}
			}
			/*int velocity_x = center_of_mass[1].x - center_of_mass[0].x;
			int velocity_y = center_of_mass[1].y - center_of_mass[0].y;
*/
			int velocity_x = position1.x - position2.x;
			int velocity_y = position1.y - position2.y;
			expected_next_position.x = center_of_mass.back().x + velocity_x;
			expected_next_position.y = center_of_mass.back().y + velocity_y;
		}

	}

};

void match_to_existing_objects(ball &current_object, vector<ball> &total_objects, int index_passed)
{
	total_objects[index_passed].contour = current_object.contour;
	//total_objects[index_passed].center_of_mass = current_objects.center_of_mass;
	total_objects[index_passed].bounding_box = current_object.bounding_box;
	total_objects[index_passed].center_of_mass.push_back(current_object.center_of_mass.back());
	total_objects[index_passed].flag = true;
	total_objects[index_passed].object_been_tracked = true;
}


void add_to_total_objects(ball &current_object, vector<ball> &total_objects)
{
	current_object.object_been_tracked = true;
	total_objects.push_back(current_object);
}


int distance_between_points(Point point1, Point point2)
{
	int x = abs(point1.x - point2.x);
	int y = abs(point1.y - point2.y);

	return(sqrt(pow(x, 2) + pow(y, 2)));
}

void match_objects(vector<ball> &total_objects, vector<ball> &current_objects)
{
	for (int i = 0; i < total_objects.size(); i++)
	{
		total_objects[i].flag = false;
		total_objects[i].next_position();
	}

	for (auto &current_object : current_objects)
	{
		int minimum_distance_index = 0;
		int distance_minimum = 10000;

		for (unsigned int k = 0; k < total_objects.size(); k++)
		{
			if (total_objects[k].object_been_tracked == true)
			{
				int distance = distance_between_points(current_object.center_of_mass.back(), total_objects[k].expected_next_position);
				if (distance < distance_minimum)
				{
					distance_minimum = distance;
					minimum_distance_index = k;
				}
			}
		}
		if (distance_minimum < 150)
			match_to_existing_objects(current_object, total_objects, minimum_distance_index);
		else
			add_to_total_objects(current_object, total_objects);
	}

	for (auto &total_object : total_objects) {

		if (total_object.flag == false) {
			total_object.NumOfConsecutiveFramesWithoutAMatch++;
		}

		if (total_object.NumOfConsecutiveFramesWithoutAMatch >= 7) 
		{
			total_object.object_been_tracked = false;
			//total_object.center_of_mass.clear();
		}

	}


}

void show_info(vector<ball> &total_objects, Mat Original_copy)
{
	for (unsigned int i = 0; i < total_objects.size(); i++) {

		if (total_objects[i].object_been_tracked == true) {
			rectangle(Original_copy, total_objects[i].bounding_box, Scalar(255, 0, 0), 2);
			putText(Original_copy, to_string(i), total_objects[i].center_of_mass.back(),2,3, Scalar(255, 0, 0), 2);
		}
	}
	imshow("Final image", Original_copy);
}
int main()
{
	cap.open("C:\\Users\\saurabh chandra\\Desktop\\video5.mp4");
	if (!cap.isOpened())
	{
		cout << "\nError in reading the file check path specified above";
		return -1;
	}
	char stop = 0;
	Mat Original_img, Original_copy;

	int hue_min = 170, hue_max = 179, sat_min = 150, sat_max = 255, val_min = 60, val_max = 255;
	bool inital_frame = true;

	namedWindow("Control", CV_WINDOW_AUTOSIZE);

	cvCreateTrackbar("LowH", "Control", &hue_min, 179);
	cvCreateTrackbar("HighH", "Control", &hue_max, 179);

	cvCreateTrackbar("LowS", "Control", &sat_min, 255);
	cvCreateTrackbar("HighS", "Control", &sat_max, 255);

	cvCreateTrackbar("LowV", "Control", &val_min, 255);
	cvCreateTrackbar("HighV", "Control", &val_max, 255);

	vector<ball> total_objects;

	while (stop != 100)
	{
		cap >> Original_img;
		Original_copy = Original_img.clone();
		cvtColor(Original_img, Original_img, COLOR_BGR2HSV);

		Mat threshold_img;

		inRange(Original_img, Scalar(hue_min, sat_min, val_min), Scalar(hue_max, sat_max, val_max), threshold_img);

		dilate(threshold_img, threshold_img, 0, Point(-1, -1), 2, 1, 1);

		imshow("Thresholded image with dilation", threshold_img);

		vector<vector<Point>> contours;

		findContours(threshold_img, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		vector<ball> current_objects;
		for (int i = 0; i < contours.size(); i++)
		{
			ball object(contours[i]);

			if (contourArea(contours[i]) > 250)
				current_objects.push_back(object);
		}

		if (inital_frame == true)
		{
			for (auto &itr:current_objects)
			{
				total_objects.push_back(itr);
			}
		}
		else
		{
			match_objects(total_objects, current_objects);
		}

		show_info(total_objects, Original_copy);

		current_objects.clear();
		inital_frame = false;

		stop = waitKey(0);

		if (stop != 27)
		{
			waitKey(10);
		}

	}
	waitKey(0);

	return 0;
}

