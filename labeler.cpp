#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <opencv2/opencv.hpp>

#include <cstdint>
#include <csignal>

std::ostream& os = std::cout;

std::set<int64_t> start_frames_full;
std::set<int64_t> end_frames_full;
std::set<int64_t> start_frames_partial;
std::set<int64_t> end_frames_partial;

void handler(int signal) {
  for(auto start : start_frames_full) {
    os << "Full Start:" << start << std::endl;
  }
  for(auto end: end_frames_full) {
    os << "Full Finish:" << end << std::endl;
  }
  for(auto start : start_frames_partial) {
    os << "Partial Start:" << start << std::endl;
  }
  for(auto end: end_frames_partial) {
    os << "Partial Finish:" << end << std::endl;
  }
}


int64_t get_prev_neighbor(std::set<int64_t>& set, int64_t frame_id) {
  int64_t best_candidate = frame_id;
  int64_t best_score = INT64_MAX;
  for(auto item : set) {
    if(item > frame_id)
      continue;
    int64_t score = frame_id - item;
    if(score < best_score) {
      best_score = score;
      best_candidate = item;
    }
  }
  return best_candidate;
}

int64_t get_next_neighbor(std::set<int64_t>& set, int64_t frame_id) {
  int64_t best_candidate = frame_id;
  int64_t best_score = INT64_MAX;
  for(auto item : set) {
    if(item < frame_id)
      continue;
    int64_t score = item - frame_id;
    if(score < best_score) {
      best_score = score;
      best_candidate = item;
    }
  }
  return best_candidate;
}

#define NONE 0
#define PARTIAL 1
#define FULL 2
int main(int argc, char* argv[]) {
  if(argc < 2) {
    std::cout << "Usage: label input_video <start_frame (optional)>" << std::endl;
  }
  std::signal(SIGINT, handler);
  std::cout << "Commands" << std::endl;
  std::cout << "Spacebar - play really fast" << std::endl;
  std::cout << "p - pause immediately" << std::endl;
  std::cout << "h - step back 1 frame" << std::endl;
  std::cout << "l - step forward 1 frame" << std::endl;
  std::cout << "n - fast forward K frames" << std::endl;
  std::cout << "b - fast backward N frames" << std::endl;
  std::cout << "] - increase fast forward speed" << std::endl;
  std::cout << "[ - decrease fast forward speed" << std::endl;
  std::cout << "s - mark the current frame as the start of a full event" << std::endl;
  std::cout << "d - mark the current frame as the end of a full event" << std::endl;
  std::cout << "a - mark the current frame as the start of a partial event" << std::endl;
  std::cout << "f - mark the current frame as the end of a partial event" << std::endl;
  std::cout << "q - quit and output all events" << std::endl;
  std::cout << "S - Go to closest full Start point" << std::endl;
  std::cout << "D - Go to closest full Finish point" << std::endl;
  std::cout << "A - Go to closest partial Start point" << std::endl;
  std::cout << "F - Go to closest partial Finish point" << std::endl;
  uint8_t marking = NONE;
  int64_t skip_interval = 60;
  int64_t frame_id = argc > 2 ? atoi(argv[2]) : 0;
  int64_t target_frame_id = frame_id;
  int64_t saved_tag = frame_id;
  cv::Point frame_number_point(20, 40);
  cv::Point start_end_point(20, 120);
  cv::Point skip_interval_point(20, 200);
  cv::Point extra_point(20, 280);
  cv::Scalar font_color(200, 200, 250);
  std::string frame_number_string = std::to_string(frame_id);
  std::string is_start_end_string = "";
  std::string skip_interval_string = "Fast-forward interval: " + std::to_string(skip_interval);
  std::string extra_text = "";
  double font_size = 1;
  cv::Mat frame;

  cv::VideoCapture vc(argv[1]);
  vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);

  cv::namedWindow("video", cv::WINDOW_AUTOSIZE);
  while(true) {
    if(frame_id < target_frame_id) {
      vc >> frame;
      frame_id += 1;
      /*frame_id += (skip_interval / 30);
      if(frame_id > target_frame_id)
        frame_id = target_frame_id;
      vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);
      vc >> frame;*/
    } else if(frame_id > target_frame_id) {
      frame_id -= (skip_interval / 30);
      if(frame_id < target_frame_id)
        frame_id = target_frame_id;
      vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);
      vc >> frame;
    } else {
      if(frame.empty()) {
        vc >> frame;
      }
    }
    if(frame.empty()) {
      break;
    }
    frame_number_string = "Frame number: " + std::to_string(frame_id) + " (" + std::to_string(target_frame_id) + ")";
    is_start_end_string = "";
    if(start_frames_full.find(frame_id) != start_frames_full.end()) {
      is_start_end_string += "Full Start";
    }
    if(end_frames_full.find(frame_id) != end_frames_full.end()) {
      is_start_end_string += "Full Finish";
    }
    if(start_frames_partial.find(frame_id) != start_frames_partial.end()) {
      is_start_end_string += "Partial Start";
    }
    if(end_frames_partial.find(frame_id) != end_frames_partial.end()) {
      is_start_end_string += "Partial Finish";
    }
    cv::putText(frame,frame_number_string, frame_number_point, CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, 2, CV_AA);
    cv::putText(frame,is_start_end_string, start_end_point, CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, 2, CV_AA);
    cv::putText(frame,skip_interval_string, skip_interval_point, CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, 2, CV_AA);
    extra_text = "";
    if(marking & PARTIAL) {
      extra_text += " (Partial) ";
    }
    if(marking & FULL) {
      extra_text += " (Full) ";
    }
    cv::putText(frame,extra_text, extra_point, CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, 2, CV_AA);
    cv::imshow("video", frame);
    auto command = cv::waitKey(1);
    if(command == -1) {
      // continue
    } else if(command == 'p') {
      target_frame_id = frame_id;
    } else if(command == 'h') {
      target_frame_id -= 1;
    } else if(command == 'l') {
      target_frame_id += 1;
    } else if(command == 'n') {
      target_frame_id += skip_interval;
    } else if(command == 'b') {
      target_frame_id -= skip_interval;
    } else if(command == '[') {
      skip_interval /= 2;
      skip_interval_string = "Fast-forward interval: " + std::to_string(skip_interval);
    } else if(command == ']') {
      skip_interval *= 2;
      skip_interval_string = "Fast-forward interval: " + std::to_string(skip_interval);
    } else if(command == 'q') {
      break;
    } else if(command == 's') {
      marking |= FULL;
      std::cout << start_frames_full.size() << std::endl;
      if(start_frames_full.find(frame_id) == start_frames_full.end())
        start_frames_full.insert(frame_id);
      else
        start_frames_full.erase(frame_id);
    } else if(command == 'd') {
      marking &= ~FULL;
      if(end_frames_full.find(frame_id) == end_frames_full.end())
        end_frames_full.insert(frame_id);
      else
        end_frames_full.erase(frame_id);
    } else if(command == 'a') {
      marking |= PARTIAL;
      std::cout << start_frames_partial.size() << std::endl;
      if(start_frames_partial.find(frame_id) == start_frames_partial.end())
        start_frames_partial.insert(frame_id);
      else
        start_frames_partial.erase(frame_id);
    } else if(command == 'f') {
      marking &= ~PARTIAL;
      if(end_frames_partial.find(frame_id) == end_frames_partial.end())
        end_frames_partial.insert(frame_id);
      else
        end_frames_partial.erase(frame_id);
    } else if(command == 'S') {
        target_frame_id = get_prev_neighbor(start_frames_full, frame_id);
        saved_tag = frame_id;
        extra_text = "(R) - return to " + std::to_string(saved_tag);
    } else if(command == 'D') {
        target_frame_id = get_next_neighbor(end_frames_full, frame_id);
        saved_tag = frame_id;
        extra_text = "(R) - return to " + std::to_string(saved_tag);
    } else if(command == 'A') {
        target_frame_id = get_prev_neighbor(start_frames_partial, frame_id);
        saved_tag = frame_id;
        extra_text = "(R) - return to " + std::to_string(saved_tag);
    } else if(command == 'F') {
        target_frame_id = get_next_neighbor(end_frames_partial, frame_id);
        saved_tag = frame_id;
        extra_text = "(R) - return to " + std::to_string(saved_tag);
    } else if(command == 'R') {
        extra_text = "";
        target_frame_id = saved_tag;
    } else if(command == ' ') {
        if(target_frame_id == INT64_MAX)
          target_frame_id = frame_id;
        else 
          target_frame_id = INT64_MAX;
    } else {
      // do nothing
    }
  }
  for(auto start : start_frames_full) {
    os << "Full Start:" << start << std::endl;
  }
  for(auto end: end_frames_full) {
    os << "Full Finish:" << end << std::endl;
  }
  for(auto start : start_frames_partial) {
    os << "Partial Start:" << start << std::endl;
  }
  for(auto end: end_frames_partial) {
    os << "Partial Finish:" << end << std::endl;
  }
}
