
#include <sys/time.h>
#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <opencv2/opencv.hpp>

#define HEIGHT 720

std::ofstream os("labels.dat");

std::vector<std::pair<int64_t, int64_t>> uncertain_intervals;
std::pair<int64_t, int64_t> cur_uncertain_interval;

std::vector<std::pair<int64_t, int64_t>> event_intervals;
std::pair<int64_t, int64_t> cur_event_interval;

std::unordered_map<int64_t, std::vector<std::string>> labels;
int64_t target_frame_number = 0;
constexpr int warning_overlay_duration = 2000;
// Variables related to the frameskip slider
constexpr int frameskip_max = 900;
int frameskip = 0;
constexpr int seek_interval_max = 900;
int seek_interval = 30;
double prev_microseconds = 0;
int64_t prev_frame_number = 0;

void display_status_text(int64_t frame_id) {
  std::string status_bar_text = "";
  if (cur_uncertain_interval.first >= 0) {
    status_bar_text +=
        "Uncertain tag start: " + std::to_string(cur_uncertain_interval.first) +
        ".";
  }
  if (cur_event_interval.first >= 0) {
    status_bar_text +=
        " Event tag start: " + std::to_string(cur_event_interval.first) + ".";
  }
  if (status_bar_text == "") {
    status_bar_text = "No tags open";
  }
  for (int i = 0; i < labels[frame_id].size(); ++i) {
    status_bar_text += " " + labels[frame_id].at(i);
  }
  cv::displayStatusBar("video", status_bar_text, 0);
}

void delete_cur_uncertain(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  labels[cur_uncertain_interval.first].pop_back();
  cur_uncertain_interval.first = -1;
  display_status_text(frame_number);
}

void delete_cur_event(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  labels[cur_event_interval.first].pop_back();
  cur_event_interval.first = -1;
  display_status_text(frame_number);
}

void uncertain_interval_start(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  // Do verification that we're not in a bad state
  if (cur_uncertain_interval.first >= 0) {
    cv::displayOverlay("video",
                       "Please close the existing uncertain interval before "
                       "starting a new one",
                       warning_overlay_duration);
  } else {
    cur_uncertain_interval.first = frame_number;
    labels[frame_number].push_back("Uncertain interval #" +
                                   std::to_string(uncertain_intervals.size()) +
                                   " start.");
  }
  display_status_text(frame_number);
}

void uncertain_interval_end(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  if (cur_uncertain_interval.first < 0) {
    cv::displayOverlay("video",
                       "Cannot end uncertain interval: start not specified",
                       warning_overlay_duration);
  } else {
    cur_uncertain_interval.second = frame_number;
    uncertain_intervals.push_back(cur_uncertain_interval);
    labels[frame_number].push_back(
        "Uncertain interval #" +
        std::to_string(uncertain_intervals.size() - 1) + " end.");
    cur_uncertain_interval.first = -1;
    cur_uncertain_interval.second = -1;
  }
  display_status_text(frame_number);
}

void event_interval_start(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  // Do verification that we're not in a bad state
  if (cur_event_interval.first >= 0) {
    cv::displayOverlay(
        "video",
        "Please close the existing event interval before starting a new one",
        warning_overlay_duration);
  } else {
    cur_event_interval.first = frame_number;
    labels[frame_number].push_back("Event interval #" +
                                   std::to_string(uncertain_intervals.size()) +
                                   " start.");
  }
  display_status_text(frame_number);
}

void event_interval_end(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  if (cur_event_interval.first < 0) {
    cv::displayOverlay("video",
                       "Cannot end event interval: start not specified",
                       warning_overlay_duration);
  } else {
    cur_event_interval.second = frame_number;
    event_intervals.push_back(cur_event_interval);
    labels[frame_number].push_back("Event interval #" +
                                   std::to_string(event_intervals.size() - 1) +
                                   " end.");
    cur_event_interval.first = -1;
    cur_event_interval.second = -1;
  }
  display_status_text(frame_number);
}

// TODO: this loop should probably be backwards for efficiency
void goto_prev_event(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  int64_t min_dist = INT64_MAX;
  if (event_intervals.size() == 0) {
    cv::displayOverlay("video", "No events.", warning_overlay_duration);
    return;
  }
  for (int i = 0; i < event_intervals.size(); ++i) {
    if (frame_number < event_intervals.at(i).first) continue;
    if (frame_number - event_intervals.at(i).first < min_dist) {
      target_frame_number = event_intervals.at(i).first;
    }
  }
  if (min_dist == INT64_MAX) {
    cv::displayOverlay("video", "You are at the first event.",
                       warning_overlay_duration);
  }
}

void goto_prev_uncertainty(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  int64_t min_dist = INT64_MAX;
  if (uncertain_intervals.size() == 0) {
    cv::displayOverlay("video", "No uncertainties.", warning_overlay_duration);
    return;
  }
  for (int i = 0; i < uncertain_intervals.size(); ++i) {
    if (frame_number < uncertain_intervals.at(i).first) continue;
    if (frame_number - uncertain_intervals.at(i).first < min_dist) {
      target_frame_number = uncertain_intervals.at(i).first;
    }
  }
  if (min_dist == INT64_MAX) {
    cv::displayOverlay("video", "You are at the first uncertainty.",
                       warning_overlay_duration);
  }
}

void goto_next_event(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  int64_t min_dist = INT64_MAX;
  if (event_intervals.size() == 0) {
    cv::displayOverlay("video", "No events.", warning_overlay_duration);
    return;
  }
  for (int i = 0; i < event_intervals.size(); ++i) {
    if (frame_number > event_intervals.at(i).first) continue;
    if (event_intervals.at(i).first - frame_number < min_dist) {
      target_frame_number = event_intervals.at(i).first;
    }
  }
  if (min_dist == INT64_MAX) {
    cv::displayOverlay("video", "You are at the last event.",
                       warning_overlay_duration);
  }
}

void goto_next_uncertainty(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  int64_t min_dist = INT64_MAX;
  if (uncertain_intervals.size() == 0) {
    cv::displayOverlay("video", "No uncertainties.", warning_overlay_duration);
    return;
  }
  for (int i = 0; i < uncertain_intervals.size(); ++i) {
    if (frame_number > uncertain_intervals.at(i).first) continue;
    if (uncertain_intervals.at(i).first - frame_number < min_dist) {
      target_frame_number = uncertain_intervals.at(i).first;
    }
  }
  if (min_dist == INT64_MAX) {
    cv::displayOverlay("video", "You are at the last uncertainty.",
                       warning_overlay_duration);
  }
}

void play_button_callback(int state, void* data) {
  target_frame_number = INT64_MAX;
}

void pause_button_callback(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  target_frame_number = frame_number;
}

void seek_forward(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  target_frame_number = frame_number + seek_interval;
}

void seek_backward(int state, void* data) {
  int64_t frame_number = *(int64_t*)data;
  target_frame_number = frame_number - seek_interval;
}

// TODO: merge the lists
void print_events(std::ostream& stream) {
  for (int i = 0; i < event_intervals.size(); i++) {
    stream << "(" << event_intervals.at(i).first << ", "
           << event_intervals.at(i).second << ") - Event " << i << std::endl;
  }
  for (int i = 0; i < uncertain_intervals.size(); i++) {
    stream << "(" << uncertain_intervals.at(i).first << ", "
           << uncertain_intervals.at(i).second << ") - Uncertain" << std::endl;
  }
}

// Gracefully handle termination
void handler(int signal) { print_events(std::cout); }

// Calculate fps
double get_fps(int64_t frame_number) {
  int64_t elapsed_frames = frame_number - prev_frame_number;
  if (elapsed_frames < 0) elapsed_frames *= -1;
  prev_frame_number = frame_number;
  struct timeval time;
  gettimeofday(&time, NULL);
  double cur_microseconds =
      double(time.tv_sec * 1000000) + double(time.tv_usec);
  double elapsed = cur_microseconds - prev_microseconds;
  prev_microseconds = cur_microseconds;
  return double(elapsed_frames) / elapsed * 1000000;
}

void print_controls() {
  std::cout << "Commands" << std::endl;
  std::cout << "Spacebar - play really fast" << std::endl;
  std::cout << "p - pause immediately" << std::endl;
  std::cout << "h - step back 1 frame" << std::endl;
  std::cout << "l - step forward 1 frame" << std::endl;
  std::cout << "n - fast forward K frames" << std::endl;
  std::cout << "b - fast backward N frames" << std::endl;
  std::cout << "a - mark the current frame as the start of a partial event"
            << std::endl;
  std::cout << "s - mark the current frame as the start of a full event"
            << std::endl;
  std::cout << "d - mark the current frame as the end of a full event"
            << std::endl;
  std::cout << "f - mark the current frame as the end of a partial event"
            << std::endl;
  std::cout << "q - quit and output all events" << std::endl;
}

cv::Mat preprocess_frame(cv::Mat frame, int64_t frame_number) {
  cv::Mat modified_frame;
  // 1. Do a resize to the target resolution
  double scale_factor = double(HEIGHT) / frame.rows;
  cv::resize(frame, modified_frame, cv::Size(0, 0), scale_factor, scale_factor,
             cv::INTER_LANCZOS4);
  // Font-related constants
  double font_size = 1;
  cv::Scalar font_color(200, 200, 250);
  int thickness = 1;

  // 2. Write fps
  cv::Point fps_point(2, 40);
  double fps = get_fps(frame_number);
  std::stringstream fps_string;
  fps_string << fps << " fps";
  cv::putText(modified_frame, fps_string.str(), fps_point,
              CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, thickness, CV_AA);

  // 2. Write frame number
  cv::Point frame_number_point(2, 80);
  std::stringstream frame_number_string;
  frame_number_string << "Frame " << frame_number;
  cv::putText(modified_frame, frame_number_string.str(), frame_number_point,
              CV_FONT_HERSHEY_SIMPLEX, font_size, font_color, thickness, CV_AA);

  return modified_frame;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: label input_video <start_frame (optional)>"
              << std::endl;
  }
  print_controls();
  std::signal(SIGINT, handler);
  int64_t frame_id = argc > 2 ? atoi(argv[2]) : 0;
  double frames_to_skip = 0;
  target_frame_number = frame_id;
  int64_t saved_tag = frame_id;
  cv::Mat frame;

  cur_uncertain_interval.first = -1;
  cur_uncertain_interval.second = -1;
  cur_event_interval.first = -1;
  cur_event_interval.second = -1;

  cv::namedWindow("video", cv::WINDOW_AUTOSIZE | CV_GUI_EXPANDED);
  cv::VideoCapture vc(argv[1]);
  vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);

  cv::createButton("uncertainty start (a)", uncertain_interval_start,
                   &frame_id);
  cv::createButton("event start (s)", event_interval_start, &frame_id);
  cv::createButton("event end (d)", event_interval_end, &frame_id);
  cv::createButton("uncertainty end (f)", uncertain_interval_end, &frame_id);
  cv::createTrackbar("Skip this many frames per 30 played", nullptr, &frameskip,
                     frameskip_max, nullptr);
  cv::createButton("Delete uncertainty start", delete_cur_uncertain, &frame_id);
  cv::createButton("Delete event start", delete_cur_event, &frame_id);
  cv::createTrackbar("Seek interval", nullptr, &seek_interval,
                     seek_interval_max, nullptr);
  cv::createButton("Play", play_button_callback);
  cv::createButton("Pause", pause_button_callback, &frame_id);
  cv::createButton("Seek forward", seek_forward, &frame_id);
  cv::createButton("Seek backward", seek_backward, &frame_id);
  int dummy = 0;
  cv::createTrackbar("Separator", nullptr, &dummy, 1, nullptr);
  cv::createButton("Go to previous event", goto_prev_event, &frame_id);
  cv::createButton("Go to previous uncertainty", goto_prev_uncertainty,
                   &frame_id);
  cv::createButton("Go to next event", goto_next_event, &frame_id);
  cv::createButton("Go to next uncertainty", goto_next_uncertainty, &frame_id);
  // cv::createButton("Return", button1_callback);
  // cv::createButton("Goto prev event", button1_callback);
  // cv::createButton("Seek to event ckpt", button1_callback);
  // Display status bar
  while (true) {
    display_status_text(frame_id);
    if (frame_id < target_frame_number) {
      if (frameskip == 0) {
        vc >> frame;
        frame_id += 1;
      } else {
        frames_to_skip += (double)frameskip / 30;
        if (frames_to_skip >= 1) {
          frame_id += frames_to_skip;
          frames_to_skip = 0;
        } else {
          frame_id += 1;
        }
        if (frame_id > target_frame_number) frame_id = target_frame_number;
        vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);
        vc >> frame;
      }
    } else if (frame_id > target_frame_number) {
      frame_id -= 1;
      frames_to_skip -= (double)frameskip / 30;
      if (frames_to_skip >= 1) {
        frame_id -= frames_to_skip;
        frames_to_skip = 0;
      } else {
        frame_id -= 1;
      }
      if (frame_id < target_frame_number) frame_id = target_frame_number;
      vc.set(cv::CAP_PROP_POS_FRAMES, frame_id);
      vc >> frame;
    } else {
      if (frame.empty()) {
        vc >> frame;
      }
    }
    if (frame.empty()) {
      target_frame_number = frame_id - 1;
    }
    cv::imshow("video", preprocess_frame(frame, frame_id));
    auto command = cv::waitKey(1);
    if (command == -1) {
      // continue
    } else if (command == 'p') {
      target_frame_number = frame_id;
    } else if (command == 'h') {
      target_frame_number -= 1;
    } else if (command == 'l') {
      target_frame_number += 1;
    } else if (command == 'n') {
      seek_forward(-1, &frame_id);
    } else if (command == 'b') {
      seek_backward(-1, &frame_id);
    } else if (command == 'q') {
      break;
    } else if (command == 's') {
      event_interval_start(-1, &frame_id);
    } else if (command == 'd') {
      event_interval_end(-1, &frame_id);
    } else if (command == 'a') {
      uncertain_interval_start(-1, &frame_id);
    } else if (command == 'f') {
      uncertain_interval_end(-1, &frame_id);
    } else if (command == ' ') {
      if (target_frame_number == INT64_MAX)
        pause_button_callback(-1, &frame_id);
      else
        play_button_callback(-1, nullptr);
    } else {
      // do nothing
    }
  }
  print_events(os);
  print_events(std::cout);
}
