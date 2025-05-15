# ✈️ PX4 + ROS 2 + micro-ROS 통합 프로젝트

이 프로젝트는 PX4 Autopilot, ROS 2, micro-ROS를 통합하여 드론의 이륙 및 착륙을 제어하는 데모를 제공합니다. 직접 설치하거나 Docker를 통해 손쉽게 체험할 수 있습니다.

## 📦 사전 요구 사항

### 1. 직접 설치 (권장)
- **PX4 Autopilot**
  - PX4 공식 문서를 참고하여 설치하세요.
- **ROS 2**
  - ROS 2 설치 가이드를 따라 설치하세요.
- **microXRCE**
  - micro-ROS 공식 문서를 참고하여 설치하세요.

### 2. Docker를 통한 체험
직접 설치가 어려운 경우, Docker를 통해 환경을 체험할 수 있습니다.
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
│   └── ...
├── .gitignore
├── .gitattributes
└── README.md
```

## 🛠️ 사용 방법

1. 위의 사전 요구 사항을 충족하거나 Docker 컨테이너를 실행합니다.
2. noVNC를 통해 GUI에 접속합니다.
3. 프로젝트의 src/ 디렉토리에서 데모를 실행합니다.
