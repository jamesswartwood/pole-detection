# Hydrology Research Tool - Pole Detection w/ Uwimg Library

---

## Project Details

Video introduction:
- [YouTube Link](https://youtu.be/a5mghKjqqSQ)

View written documentation here:
- [jamesswartwood.github.io/pole-detection/](https://jamesswartwood.github.io/pole-detection/)

---

## Quick Start Guide

1. Clone this repository to your local desktop
2. Navigate to the repository in your terminal
3. Build the executable files by running the `make` command
4. If you do not already have python on your machine, download it now
5. Run a python script with `python <filename>`
    - `get_points.py` will output a .csv file
    - `get_annotations.py` will output all of the annotated images
6. These scripts are set to run on the images in `data/samples` and output to `output/samples` with no prefix by default. You can update `data_path`, `save_path`, and `save_prefix` to different values in the python script files themselves.

---

## Acknowledgements

The [uwing](https://github.com/pjreddie/uwimg/) library was developed over the course of [CSE 455](https://courses.cs.washington.edu/courses/cse455/22sp/). Everything in `src/project` was developed for this project specifically, and the key functions and additional data structures were inserted into the uwing library for compiling purposes.

---

If you have any questions, feel free to email **jamesmw@uw.edu**