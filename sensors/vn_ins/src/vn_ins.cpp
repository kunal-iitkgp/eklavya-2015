#include <vn_ins.hpp>
#include <LifeCycle.hpp>

vectorNav::vectorNav(int argc, char** argv) : Sensor(argc, argv) {
    ros::init(argc, argv, node_name.c_str());
    node_handle = new ros::NodeHandle();
    node_handle->getParam("imu_only", imu_only);
    if (argc > 1) {
        initializeParameters(argc, argv);
    } else {
        initializeParameters();
    }
    setupCommunications();
}

vectorNav::~vectorNav() {
}

bool vectorNav::connect() {
    // ROS_INFO("vn100_com_port %s",vn100_com_port.c_str());
      ROS_INFO("vn200_com_port %s",vn200_com_port.c_str());
    //if (!imu_only) {
        vn200_connect(&vn200, vn200_com_port.c_str(), baud_rate);
    //}
    //vn100_connect(&vn100, vn100_com_port.c_str(), baud_rate);
    return true;
}

bool vectorNav::disconnect() {
    //if (!imu_only) {
      //  vn200_disconnect(&vn200);
    //}
    vn200_disconnect(&vn200);
    return true;
}

bool vectorNav::fetch() {
    node_handle->getParam("imu_only", imu_only);

    unsigned short gpsWeek, status;
    float yaw;
    VnYpr vn200_attitude;
    VnVector3 vn200_magnetic, vn200_acceleration, vn200_angular_rate;
    vn200_getYawPitchRoll(&vn200, &vn200_attitude);
    yaw = vn200_attitude.yaw;
    vn200_getYawPitchRollMagneticAccelerationAngularRate(&vn200, &vn200_attitude, &vn200_magnetic, &vn200_acceleration, &vn200_angular_rate);
    _twist.angular.z=vn200_angular_rate.c2;
    if (!imu_only) {
        unsigned char gpsFix, numberOfSatellites;
        float speedAccuracy, timeAccuracy, attitudeUncertainty, positionUncertainty, velocityUncertainty, temperature, pressure;
        double gpsTime, latitude, longitude, altitude;
        VnVector3 magnetic, acceleration, angularRate, ypr, latitudeLognitudeAltitude, nedVelocity, positionAccuracy;

        vn200_getGpsSolution(&vn200, &gpsTime, &gpsWeek, &gpsFix, &numberOfSatellites, &latitudeLognitudeAltitude, &nedVelocity, &positionAccuracy, &speedAccuracy, &timeAccuracy);
        ROS_INFO("Triangulating from %d satellites", numberOfSatellites);
        /*vn200_getInsState(&vn200, &vn200_attitude, &latitudeLognitudeAltitude, &nedVelocity, &vn200_acceleration, &vn200_angular_rate);

        yaw = vn200_attitude.yaw;

        _twist.angular.z=vn200_angular_rate.c2;*/

        _gps.latitude = latitudeLognitudeAltitude.c0;
        _gps.longitude = latitudeLognitudeAltitude.c1;
        _gps.altitude = latitudeLognitudeAltitude.c2;
    }

    _yaw.data = -yaw;
    double th=_yaw.data * (3.14159265/180);
    _imu.orientation=tf::createQuaternionMsgFromYaw(th);
    _imu.angular_velocity.z=(-1)*(_twist.angular.z);
    _imu.linear_acceleration.x=vn200_acceleration.c0;
    _imu.linear_acceleration.y=vn200_acceleration.c1;
    _imu.linear_acceleration.z=vn200_acceleration.c2;

    return true;
}

void vectorNav::publish(int frame_id) {
    if (!imu_only) {
        fix_publisher.publish(_gps);
    }
    //yaw_publisher.publish(_yaw);
    twist_pub.publish(_twist);
    imu_pub.publish(_imu);
}

void vectorNav::initializeParameters() {
    baud_rate = 115200;
    message_queue_size = 10;
    node_name = std::string("vn_ins");
    fix_topic_name = node_name + std::string("/fix");
    //yaw_topic_name = node_name + std::string("/yaw");
    twist_topic_name = node_name + std::string("/twist");
    imu_topic_name = node_name + std::string("/imu");
   //vn100_com_port = std::string("/dev/serial/by-id/usb-FTDI_USB-RS232_Cable_FTVJUC0O-if00-port0");
   vn200_com_port = std::string("/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_AH02V7ZW-if00-port0");//usb-FTDI_USB-RS232_Cable_FTVJUC0O-if00-port0");
    
    node_handle->getParam("baud_rate", baud_rate);
    node_handle->getParam("message_queue_size", message_queue_size);
    node_handle->getParam("node_name", node_name);
    node_handle->getParam("fix_topic_name", fix_topic_name);
    node_handle->getParam("yaw_topic_name", yaw_topic_name);
    node_handle->getParam("twist_topic_name", twist_topic_name);
    node_handle->getParam("imu_topic_name", imu_topic_name);
    //node_handle->getParam("vn100_com_port", vn100_com_port);
    node_handle->getParam("vn200_com_port", vn200_com_port);
   
}

void vectorNav::initializeParameters(int argc, char** argv) {
    baud_rate = 115200;
    message_queue_size = 10;
    node_name = std::string("/vn_ins") + std::string(argv[1]);
    fix_topic_name = node_name + std::string(argv[1]) + std::string("/fix");
    //yaw_topic_name = node_name + std::string(argv[1]) + std::string("/yaw");
    twist_topic_name = node_name + std::string(argv[1]) + std::string("/twist");
    imu_topic_name = node_name + std::string(argv[1]) + std::string("/imu");

    vn200_com_port = std::string(argv[2]);
    //vn100_com_port = std::string(argv[3]);
}

void vectorNav::setupCommunications() {
    fix_publisher = node_handle->advertise<sensor_msgs::NavSatFix>(fix_topic_name.c_str(), message_queue_size);
    //yaw_publisher = node_handle->advertise<std_msgs::Float64>(yaw_topic_name.c_str(), message_queue_size);
    twist_pub = node_handle->advertise<geometry_msgs::Twist>(twist_topic_name.c_str(), message_queue_size);
    imu_pub = node_handle->advertise<sensor_msgs::Imu>(imu_topic_name.c_str(), message_queue_size);
}

int main(int argc, char** argv) {
    if (argc > 1 && argc <= 3) {
        printf("Usage : <name> <id_no> <COM_PORT>\n");
    }

    double loop_rate = 40;
    int frame_id;
    frame_id = 0;
    vectorNav *vectornav = new vectorNav(argc, argv);
    vectornav->connect();
    ROS_INFO("VectorNav successfully connected. \n");
    //taking average-----------
    /*int i=0,iter=20;
    float sum=0.0;
    for(;i<iter;i++)
    {
        vectornav->fetch();
        sum+=vectornav->_yaw.data;
        usleep(50000);
    }
    yaw_init=sum/i;*/
    //--------------------------
    ros::Rate rate_enforcer(loop_rate);
    while (ros::ok()) {
        vectornav->fetch();
        vectornav->publish(frame_id);
        ros::spinOnce();
        rate_enforcer.sleep();
    }

    vectornav->disconnect();
    return 0;
}
