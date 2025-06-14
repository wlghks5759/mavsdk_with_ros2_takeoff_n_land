# ✈️ PX4 + ROS 2 + MAVSDK 간단한 예제 프로젝트 (ARM + TAKEOFF + LAND)

![image](https://github.com/user-attachments/assets/6e364031-6178-4da4-9d18-cb48672b702e)

이 프로젝트는 PX4 Autopilot, ROS 2, micro-ROS를 통합하여 드론의 이륙 및 착륙을 제어하는 데모를 제공합니다. 직접 설치하거나 Docker를 통해 손쉽게 사용할 수 있습니다.

## 📦 사전 요구 사항

### 1. 직접 설치 (권장)
- **PX4 Autopilot**
  - PX4 공식 문서를 참고하여 설치하세요.
- **ROS 2**
  - ROS 2 설치 가이드를 따라 설치하세요.
- **microXRCE**
  - microXRCE 공식 문서를 참고하여 설치하세요.
- **MAVSDK**
  - MAVSDK 공식 문서를 참고하여 설치하세요.

### 2. Docker를 통한 사용
직접 설치가 어려운 경우, Docker를 통해 환경을 사용할 수 있습니다.
```bash
docker pull holytorch/px4_ros2_mavsdk
```

## 🚀 Docker로 실행하기

아래 명령어로 Docker 컨테이너를 실행하세요:
```bash
docker run -e RESOLUTION=1440x900 -p 6080:6080 holytorch/px4_ros2_mavsdk
```
- **RESOLUTION**: 원하는 해상도로 설정 가능합니다 (예: 1920x1080).
- **-p 6080:6080**: 웹 브라우저를 통해 접속할 수 있도록 포트를 매핑합니다.

## 🌐 noVNC를 통한 접속

컨테이너 실행 후, 웹 브라우저에서 아래 주소로 접속하여 GUI를 사용할 수 있습니다:
```
http://localhost:6080/vnc.html
```

> ⚠️ **접속이 되지 않을 경우, 다른 웹 브라우저(예: Chrome, Firefox)를 사용해 보세요.**

## 📁 프로젝트 구조

```
mavsdk_with_ros2_takeoff_n_land/
├── src/
│   └── takeoff_and_land_ros2/
│       ├── CMakeLists.txt
│       ├── package.xml
│       ├── launch/
│       │   └── takeoff_land.launch.py
│       └── src/
│           └── takeoff_and_land_node.cpp
├── .gitignore
├── .gitattributes
└── README.md
```

## 🛠️ 사용 방법

1. 위의 사전 요구 사항을 충족하거나 Docker 컨테이너를 실행합니다.
2. (noVNC일경우)noVNC를 통해 GUI에 접속합니다.
3. 홈 디렉토리로 이동해 프로젝트 클론:
   ```bash
   cd ~
   git clone https://github.com/wlghks5759/mavsdk_with_ros2_takeoff_n_land.git
   cd ~/mavsdk_with_ros2_takeoff_n_land
   colcon build
   ```

4. 다음 세 개의 터미널에서 각각 실행:

   **첫 번째 터미널 (PX4 시뮬레이터)**
   ```bash
   cd ~/PX4-Autopilot
   make px4_sitl gz_x500
   ```

   **두 번째 터미널 (Micro-XRCE-DDS Agent)**
   ```bash
   cd ~/Micro-XRCE-DDS-Agent
   MicroXRCEAgent udp4 -p 8888
   ```

   **세 번째 터미널 (ROS 2 노드)**
   ```bash
   cd ~/mavsdk_with_ros2_takeoff_n_land
   colcon build
   source install/setup.bash
   ros2 launch takeoff_and_land_ros2 takeoff_land.launch.py
   ```

   **네 번째 터미널 (서비스 호출)**
   ```bash
   cd ~/mavsdk_with_ros2_takeoff_n_land
   source install/setup.bash
   ```

5. PX4 SITL이 켜진 상태에서 네 번째 터미널에서 다음 ROS 2 서비스 호출 명령들을 순서대로 입력하세요:

   **1단계: 드론 Arm 시키기**
   ```bash
   ros2 service call /arm_vehicle std_srvs/srv/Trigger "{}"
   ```
   이 명령을 실행하면 첫 번째 터미널(노드 로그가 나오는 곳)에 "Arming..." 같은 메시지가 뜨고, 성공하면 서비스 응답으로 `success: true`가 보일 것입니다. 시뮬레이터의 드론 상태도 Armed로 바뀔 수 있습니다.

   **2단계: 드론 이륙 시키기**
   Arm이 성공적으로 완료된 후 다음 명령을 실행합니다.
   ```bash
   ros2 service call /takeoff_vehicle std_srvs/srv/Trigger "{}"
   ```
   첫 번째 터미널에 "Taking off..." 메시지가 뜨고, 시뮬레이터의 드론이 실제로 이륙을 시작해야 합니다. 고도 정보를 보고 싶다면 또 다른 새 터미널에서 다음을 실행할 수 있습니다:
   ```bash
   ros2 topic echo /altitude
   ```

   **3단계: (선택 사항) 잠시 호버링**
   코드가 자동으로 호버링 후 착륙하는 로직은 서비스 내에 직접적으로 넣지 않았습니다. 이륙 후 잠시 기다렸다가 다음 명령을 실행하세요.

   **4단계: 드론 착륙 시키기**
   ```bash
   ros2 service call /land_vehicle std_srvs/srv/Trigger "{}"
   ```

6. 다른 연결 URL 사용 방법:

   **시리얼 포트 사용 시** (예: 실제 드론의 시리얼 포트)
   ```bash
   ros2 launch takeoff_and_land_ros2 takeoff_land.launch.py connection_url:=serial:///dev/ttyACM0:57600
   ```

   **TCP 연결 사용 시**
   ```bash
   ros2 launch takeoff_and_land_ros2 takeoff_land.launch.py connection_url:=tcp://<DRONE_IP>:5760
   ```

   
