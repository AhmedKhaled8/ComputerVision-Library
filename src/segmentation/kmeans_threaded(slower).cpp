#include <iostream>
#include <random>
#include <cmath>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <chrono>
#include <thread>
#include "../../include/segmentation/kmeans.h"

using namespace std::chrono;
using std::cout, std::endl;
using std::thread;

void print_centroids(std::vector<BGR_point> centroids){
    for(auto & centroid : centroids){
        std::cout << "(" << centroid.b << "," << centroid.g << ","  << centroid.r << "), ";
    }
}

double calculate_euclidean_distance(BGR_point p1, BGR_point p2){
    /* When Changing this remember to change #define MIN_CENTROID_DISTANCE 100 if needed */
    double b_distance = (p1.b - p2.b) * (p1.b - p2.b);
    double g_distance = (p1.g - p2.g) * (p1.g - p2.g);
    double r_distance = (p1.r - p2.r) * (p1.r - p2.r);
    double distance = b_distance + g_distance + r_distance;
    return distance;
}

std::vector<BGR_point> generate_random_centroids(int number_of_centroids, cv::Mat image){
  auto start = high_resolution_clock::now();
    std::random_device rd;
    std::default_random_engine random_generator(rd());
    std::uniform_int_distribution<int> rows_distribution(0, image.rows);
    std::uniform_int_distribution<int> columns_distribution(0, image.cols);
    std::vector<BGR_point> centroids;
    for(int centroid_index = 0; centroid_index < number_of_centroids; centroid_index++){
        bool centroid_far;
        BGR_point centroid;
        do {
            centroid_far = true;
            // std::cout << "Generating New Random Centroid ... ";
            int row = rows_distribution(random_generator);
            int col = columns_distribution(random_generator);
            centroid.b = image.at<CV_BGR_VECTOR>(row, col)[BLUE];
            centroid.g = image.at<CV_BGR_VECTOR>(row, col)[GREEN];
            centroid.r = image.at<CV_BGR_VECTOR>(row, col)[RED];
            // std::cout << "(" << centroid.b << "," << centroid.g << "," << centroid.r << ")" << std::endl;
            for (int i = 0; i < centroids.size(); ++i) {
                double distance = calculate_euclidean_distance(centroid, centroids[i]);
                // std::cout << "Distance: " << distance << std::endl;
                if (distance <= MIN_CENTROID_DISTANCE) {
                    // std::cout << "Distance is less than 5" << std::endl;
                    centroid_far = false;
                    break;
                }
            }
        }
        while(!centroid_far);
        // std::cout << "Centroid Pushed" << std::endl;
        centroids.push_back(centroid);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Random centroids: " << duration.count() << endl;
    return centroids;
}

std::vector<BGR_point> create_image_data(cv::Mat image){
  auto start = high_resolution_clock::now();
    int rows = image.rows;
    int cols = image.cols;
    std::vector<BGR_point> image_data;
    BGR_point point;
    for(int r = 0; r < rows; r++){
        for(int c = 0; c < cols; c++){
            point.b = image.at<CV_BGR_VECTOR>(r, c)[BLUE];
            point.g = image.at<CV_BGR_VECTOR>(r, c)[GREEN];
            point.r = image.at<CV_BGR_VECTOR>(r, c)[RED];
            image_data.push_back(point);
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Create image data: " << duration.count() << endl;
    return image_data;
}


int get_closest_cluster(BGR_point point, std::vector<BGR_point> centroids){
    double min_distance = calculate_euclidean_distance(point, centroids[0]);
    int closest_cluster = 0;
    double distance;
    for(int cluster = 1; cluster < centroids.size(); cluster++){
        distance = calculate_euclidean_distance(point, centroids[cluster]);
        if(distance < min_distance){
            min_distance = distance;
            closest_cluster = cluster;
        }
    }
    return closest_cluster;
}

std::vector<int> assign_clusters(std::vector<BGR_point> image_data, std::vector<BGR_point> centroids){
  auto start = high_resolution_clock::now();
    int n_samples = image_data.size();
    int cluster_index = 0;
    std::vector<int> clusters_vector;
    for(int i = 0; i < n_samples; i++){
        cluster_index = get_closest_cluster(image_data[i], centroids);
        clusters_vector.push_back(cluster_index);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << duration.count() << endl;
    return clusters_vector;
}

void update_point(std::vector<BGR_point> *image_data, std::vector<int> *clusters, int *point_dim, int *Cluster_Count, int c, int channel_index)
{
  int cluster_count = 0;
  for(int im = 0; im < (*image_data).size(); im++){
      if((*clusters)[im] == c){
          *point_dim += (*image_data)[im][channel_index];
          cluster_count++;
      }
  }
  *Cluster_Count = cluster_count;
}

std::vector<BGR_point> update_centroids(std::vector<BGR_point> image_data, std::vector<int> clusters, std::vector<BGR_point> centroids){
  auto start = high_resolution_clock::now();
    std::vector<BGR_point> new_centroids;
    for(int c = 0; c < centroids.size(); c++){
        BGR_point point;
        point.b = 0;
        point.g = 0;
        point.r = 0;
        int cluster_count = 0;
        int holders[3] = {0};
        int Cluster_Count[3] = {0};
        // update_point;
        thread up1 (update_point, &image_data, &clusters, &point.b, &Cluster_Count[0], c, 0);
        thread up2 (update_point, &image_data, &clusters, &point.g, &Cluster_Count[1], c, 1);
        thread up3 (update_point, &image_data, &clusters, &point.r, &Cluster_Count[2], c, 2);

        up1.join();
        up2.join();
        up3.join();
        // update_point(&image_data, &clusters, &point.b, &Cluster_Count[0], c, 0);
        // update_point(&image_data, &clusters, &point.g, &Cluster_Count[1], c, 1);
        // update_point(&image_data, &clusters, &point.r, &Cluster_Count[2], c, 2);
        cluster_count = Cluster_Count[0];
        if(cluster_count > 0){
            point.b /= cluster_count;
            point.g /= cluster_count;
            point.r /= cluster_count;
            new_centroids.push_back(point);
        }
        else{
            new_centroids.push_back(centroids[c]);
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Update centroids: " << duration.count() << endl;
    return new_centroids;
}

bool is_converged(std::vector<BGR_point> old_centroids, std::vector<BGR_point> new_centroids){
    double sum = 0.0;
    for(int cent_idx=0; cent_idx < old_centroids.size(); ++cent_idx){
        sum += std::pow((old_centroids[cent_idx].b - new_centroids[cent_idx].b), 2) + std::pow((old_centroids[cent_idx].g - new_centroids[cent_idx].g), 2) + std::pow((old_centroids[cent_idx].r - new_centroids[cent_idx].r), 2);
    }
    return sum < 1;
}

void segment_image(cv::Mat image, KMeans_result &kmeans){
    cv::Mat segmented_image = image.clone();
    int cols = image.cols;
    int rows = image.rows;
    for(int r = 0; r < rows; ++r){
        for(int c = 0; c < cols; ++c){
            segmented_image.at<CV_BGR_VECTOR>(r, c)[BLUE] = kmeans.centroids[kmeans.clusters[r * cols + c]].b;
            segmented_image.at<CV_BGR_VECTOR>(r, c)[GREEN] = kmeans.centroids[kmeans.clusters[r * cols + c]].g;
            segmented_image.at<CV_BGR_VECTOR>(r, c)[RED] = kmeans.centroids[kmeans.clusters[r * cols + c]].r;
        }
    }
    kmeans.segmented_image = segmented_image;
}

KMeans_result apply_kmeans(cv::Mat imageBGR, int num_clusters, int max_iterations){
    cv::Mat image;
    if(imageBGR.channels() != 3){
        cv::cvtColor(imageBGR, image, cv::COLOR_GRAY2BGR);
    }
    else{
        image = imageBGR.clone();
    }
    std::vector<BGR_point> centroids = generate_random_centroids(num_clusters, image);
    std::vector<BGR_point> image_data = create_image_data(image);
    std::vector<int> clusters;
    std::vector<BGR_point> old_centroids;
    for(int iter = 1; iter <= max_iterations; ++iter){
        clusters = assign_clusters(image_data, centroids);
        old_centroids = centroids;
        centroids = update_centroids(image_data, clusters, centroids);
        if(is_converged(old_centroids, centroids)){
            std::cout << "Converged at iteration " << iter << std::endl;
            break;
        }
    }
    KMeans_result kmeans_result;
    kmeans_result.clusters = clusters;
    kmeans_result.centroids = centroids;
    segment_image(image, kmeans_result);
    return kmeans_result;
}
