# SmartFresh_network_group-55
# SmartFresh Network: AI & IoT-Based Tomato Quality and Spoilage Detection System

## Project Title

**SmartFresh Network: AI & IoT-Based Tomato Quality and Spoilage Detection System**

## Problem & Solution

### Problem

Tomato spoilage is a major challenge faced by vendors and farmers due to poor quality assessment methods and unfavorable storage conditions. Many vendors depend on manual visual inspection to determine whether tomatoes are fresh or spoiled, which can result in inaccurate decisions, food wastage, and financial losses.

Additionally, changes in environmental conditions such as temperature and humidity can accelerate tomato deterioration. There is a need for an affordable and reliable system that can assist vendors in monitoring tomato quality and making better decisions.

### Solution

SmartFresh Network is an AI and IoT-based tomato quality and spoilage detection system designed to identify tomato freshness and monitor environmental conditions.

The system uses an ESP32-CAM module to capture tomato images, an AI model developed using Edge Impulse to classify tomato quality, and a temperature and humidity sensor to monitor storage conditions. The collected information is processed and displayed through a web-based dashboard, allowing users to easily monitor tomato quality status.

The system helps reduce post-harvest losses by providing faster and more accurate tomato quality assessment compared to traditional manual inspection methods.

---

# System Features

* AI-based tomato freshness and spoilage detection
* Image capture using ESP32-CAM
* Temperature and humidity monitoring
* Real-time display of tomato quality results
* Web-based monitoring dashboard
* IoT connectivity for remote data access
* Improved decision-making for tomato vendors

---

# Technologies Used

## Hardware Components

* ESP32-CAM Module
* SHTC3 Temperature and Humidity Sensor
* Push Button
* Jumper Wires
* Breadboard

## Software Technologies

* Arduino IDE
* Edge Impulse Machine Learning Platform
* HTML
* CSS
* Firebase Realtime Database

---

# System Requirements

## Hardware Requirements

* Computer/laptop
* ESP32-CAM board
* Required sensors and electronic components
* USB cable
* FTDI programmer

## Software Requirements

* Arduino IDE installed
* ESP32 board package configured
* Required Arduino libraries installed
* Modern web browser
* Code editor such as Visual Studio Code

---

# Setup Instructions

## 1. Clone the Repository

Download or clone this repository to your computer:

```
git clone [GitHub Repository Link]
```

Navigate into the project folder:

```
cd SmartFresh-Network
```

---

# 2. Arduino Setup

1. Install Arduino IDE.

2. Open Arduino IDE.

3. Install ESP32 board support.

4. Install the required libraries:

   * ESP32 Camera Library
   * Sensor libraries
   * Firebase libraries

5. Open the Arduino source code located in:

```
Arduino/SmartFresh_ESP32_Code.ino
```

6. Configure the required settings:

   * WiFi credentials
   * Database configuration

7. Connect the ESP32-CAM using the FTDI programmer.

8. Upload the code to the ESP32-CAM.

---

# 3. Dashboard Setup

1. Open the Dashboard folder:

```
Dashboard/
```

2. Open the files using Visual Studio Code or another code editor.

3. Configure the database connection settings.

4. Run the dashboard using a local web server.

5. Open the dashboard in a web browser.

---

# Project Structure

```
SmartFresh-Network/

│
├── Arduino/
│   └── SmartFresh_ESP32_Code.ino
│
├── Dashboard/
│   ├── index.html
│   ├── style.css
│   
│
├── Images/
│   └── Project screenshots
│
└── README.md
```

---

# How the System Works

1. A tomato sample is placed in front of the ESP32-CAM.
2. The user presses the capture button.
3. The ESP32-CAM captures an image of the tomato.
4. The AI model processes the image and determines tomato quality.
5. The temperature and humidity sensor collects environmental data.
6. The system sends the results to the dashboard.
7. The user views tomato quality information through the dashboard.

---

# System Workflow

```
Tomato Sample
      |
      ↓
ESP32-CAM Image Capture
      |
      ↓
AI Model Classification
      |
      ↓
Temperature & Humidity Monitoring
      |
      ↓
Data Processing
      |
      ↓
Dashboard Display
```

---

# Applications

* Tomato vendors
* Small-scale farmers
* Agricultural monitoring systems
* Food quality management
* Smart farming solutions

---

# Future Improvements

* Add SMS/email alerts for spoiled tomatoes
* Improve AI model accuracy with more training images
* Develop a mobile application
* Add cloud-based analytics
* Expand detection to other fruits and vegetables

---

# Contributors
 STUDENT NAME	REGISTRATION NO.
1.	KISAKYE ESTHER	2024/DCS/DAY/2023/G
2.	WAISWA GAVIN	2024/DCS/EVE/0817
3.	SSEKUUBWA MARK TIMOTHY	2024/DCS/DAY/0251 
4.	OTHIENO ROGERS JAYEE	2024/DCS/DAY/0922/G
5.	  KALYANGO DOUGLAS	2024/ITB/DAY/1829/G 
---

