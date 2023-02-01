/*
    FILE: dynamicDetector.cpp
    ---------------------------------
    function implementation of dynamic osbtacle detector
*/
#include <map_manager/detector/dynamicDetector.h>


namespace mapManager{
    dynamicDetector::dynamicDetector(){
        this->ns_ = "dynamic_detector";
        this->hint_ = "[dynamicDetector]";
    }

    dynamicDetector::dynamicDetector(const ros::NodeHandle& nh){
        this->ns_ = "dynamic_detector";
        this->hint_ = "[dynamicDetector]";
        this->nh_ = nh;
        this->initParam();
        this->registerPub();
        this->registerCallback();
    }

    void dynamicDetector::initDetector(const ros::NodeHandle& nh){
        this->nh_ = nh;
        this->initParam();
        this->registerPub();
        this->registerCallback();
    }

    void dynamicDetector::initParam(){
        // localization mode
        if (not this->nh_.getParam(this->ns_ + "/localization_mode", this->localizationMode_)){
            this->localizationMode_ = 0;
            cout << this->hint_ << ": No localization mode option. Use default: pose" << endl;
        }
        else{
            cout << this->hint_ << ": Localizaiton mode: pose (0)/odom (1). Your option: " << this->localizationMode_ << endl;
        }   

        // depth topic name
        if (not this->nh_.getParam(this->ns_ + "/depth_image_topic", this->depthTopicName_)){
            this->depthTopicName_ = "/camera/depth/image_raw";
            cout << this->hint_ << ": No depth image topic name. Use default: /camera/depth/image_raw" << endl;
        }
        else{
            cout << this->hint_ << ": Depth topic: " << this->depthTopicName_ << endl;
        }

        // aligned depth topic name
        if (not this->nh_.getParam(this->ns_ + "/aligned_depth_image_topic", this->alignedDepthTopicName_)){
            this->alignedDepthTopicName_ = "/camera/aligned_depth_to_color/image_raw";
            cout << this->hint_ << ": No aligned depth image topic name. Use default: /camera/aligned_depth_to_color/image_raw" << endl;
        }
        else{
            cout << this->hint_ << ": Aligned depth topic: " << this->alignedDepthTopicName_ << endl;
        }

        if (this->localizationMode_ == 0){
            // odom topic name
            if (not this->nh_.getParam(this->ns_ + "/pose_topic", this->poseTopicName_)){
                this->poseTopicName_ = "/CERLAB/quadcopter/pose";
                cout << this->hint_ << ": No pose topic name. Use default: /CERLAB/quadcopter/pose" << endl;
            }
            else{
                cout << this->hint_ << ": Pose topic: " << this->poseTopicName_ << endl;
            }           
        }

        if (this->localizationMode_ == 1){
            // pose topic name
            if (not this->nh_.getParam(this->ns_ + "/odom_topic", this->odomTopicName_)){
                this->odomTopicName_ = "/CERLAB/quadcopter/odom";
                cout << this->hint_ << ": No odom topic name. Use default: /CERLAB/quadcopter/odom" << endl;
            }
            else{
                cout << this->hint_ << ": Odom topic: " << this->odomTopicName_ << endl;
            }
        }

        std::vector<double> depthIntrinsics (4);
        if (not this->nh_.getParam(this->ns_ + "/depth_intrinsics", depthIntrinsics)){
            cout << this->hint_ << ": Please check camera intrinsics!" << endl;
            exit(0);
        }
        else{
            this->fx_ = depthIntrinsics[0];
            this->fy_ = depthIntrinsics[1];
            this->cx_ = depthIntrinsics[2];
            this->cy_ = depthIntrinsics[3];
            cout << this->hint_ << ": fx, fy, cx, cy: " << "["  << this->fx_ << ", " << this->fy_  << ", " << this->cx_ << ", "<< this->cy_ << "]" << endl;
        }

        // depth scale factor
        if (not this->nh_.getParam(this->ns_ + "/depth_scale_factor", this->depthScale_)){
            this->depthScale_ = 1000.0;
            cout << this->hint_ << ": No depth scale factor. Use default: 1000." << endl;
        }
        else{
            cout << this->hint_ << ": Depth scale factor: " << this->depthScale_ << endl;
        }

        // depth min value
        if (not this->nh_.getParam(this->ns_ + "/depth_min_value", this->depthMinValue_)){
            this->depthMinValue_ = 0.2;
            cout << this->hint_ << ": No depth min value. Use default: 0.2 m." << endl;
        }
        else{
            cout << this->hint_ << ": Depth min value: " << this->depthMinValue_ << endl;
        }

        // depth max value
        if (not this->nh_.getParam(this->ns_ + "/depth_max_value", this->depthMaxValue_)){
            this->depthMaxValue_ = 5.0;
            cout << this->hint_ << ": No depth max value. Use default: 5.0 m." << endl;
        }
        else{
            cout << this->hint_ << ": Depth depth max value: " << this->depthMaxValue_ << endl;
        }

        // depth filter margin
        if (not this->nh_.getParam(this->ns_ + "/depth_filter_margin", this->depthFilterMargin_)){
            this->depthFilterMargin_ = 0;
            cout << this->hint_ << ": No depth filter margin. Use default: 0." << endl;
        }
        else{
            cout << this->hint_ << ": Depth filter margin: " << this->depthFilterMargin_ << endl;
        }

        // depth skip pixel
        if (not this->nh_.getParam(this->ns_ + "/depth_skip_pixel", this->skipPixel_)){
            this->skipPixel_ = 1;
            cout << this->hint_ << ": No depth skip pixel. Use default: 1." << endl;
        }
        else{
            cout << this->hint_ << ": Depth skip pixel: " << this->skipPixel_ << endl;
        }

        // ------------------------------------------------------------------------------------
        // depth image columns
        if (not this->nh_.getParam(this->ns_ + "/image_cols", this->imgCols_)){
            this->imgCols_ = 640;
            cout << this->hint_ << ": No depth image columns. Use default: 640." << endl;
        }
        else{
            cout << this->hint_ << ": Depth image columns: " << this->imgCols_ << endl;
        }

        // depth skip pixel
        if (not this->nh_.getParam(this->ns_ + "/image_rows", this->imgRows_)){
            this->imgRows_ = 480;
            cout << this->hint_ << ": No depth image rows. Use default: 480." << endl;
        }
        else{
            cout << this->hint_ << ": Depth image rows: " << this->imgRows_ << endl;
        }
        this->projPoints_.resize(this->imgCols_ * this->imgRows_ / (this->skipPixel_ * this->skipPixel_));
        this->pointsDepth_.resize(this->imgCols_ * this->imgRows_ / (this->skipPixel_ * this->skipPixel_));
        // ------------------------------------------------------------------------------------


        // transform matrix: body to camera
        std::vector<double> body2CamVec (16);
        if (not this->nh_.getParam(this->ns_ + "/body_to_camera", body2CamVec)){
            ROS_ERROR("[dynamicDetector]: Please check body to camera matrix!");
        }
        else{
            for (int i=0; i<4; ++i){
                for (int j=0; j<4; ++j){
                    this->body2Cam_(i, j) = body2CamVec[i * 4 + j];
                }
            }
        }
        
        std::vector<double> colorIntrinsics (4);
        if (not this->nh_.getParam(this->ns_ + "/color_intrinsics", colorIntrinsics)){
            cout << this->hint_ << ": Please check camera intrinsics!" << endl;
            exit(0);
        }
        else{
            this->fxC_ = colorIntrinsics[0];
            this->fyC_ = colorIntrinsics[1];
            this->cxC_ = colorIntrinsics[2];
            this->cyC_ = colorIntrinsics[3];
            cout << this->hint_ << ": fxC, fyC, cxC, cyC: " << "["  << this->fxC_ << ", " << this->fyC_  << ", " << this->cxC_ << ", "<< this->cyC_ << "]" << endl;
        }

        // transform matrix: body to camera color
        std::vector<double> body2CamColorVec (16);
        if (not this->nh_.getParam(this->ns_ + "/body_to_camera_color", body2CamColorVec)){
            ROS_ERROR("[dynamicDetector]: Please check body to camera color matrix!");
        }
        else{
            for (int i=0; i<4; ++i){
                for (int j=0; j<4; ++j){
                    this->body2CamColor_(i, j) = body2CamColorVec[i * 4 + j];
                }
            }
        }

        // Raycast max length
        if (not this->nh_.getParam(this->ns_ + "/raycast_max_length", this->raycastMaxLength_)){
            this->raycastMaxLength_ = 5.0;
            cout << this->hint_ << ": No raycast max length. Use default: 5.0." << endl;
        }
        else{
            cout << this->hint_ << ": Raycast max length: " << this->raycastMaxLength_ << endl;
        }

        // ground height
        if (not this->nh_.getParam(this->ns_ + "/ground_height", this->groundHeight_)){
            this->groundHeight_ = 0.1;
            std::cout << this->hint_ << ": No ground height parameter. Use default: 0.1m." << std::endl;
        }
        else{
            std::cout << this->hint_ << ": Ground height is set to: " << this->groundHeight_ << std::endl;
        }

        // minimum number of points in each cluster
        if (not this->nh_.getParam(this->ns_ + "/dbscan_min_points_cluster", this->dbMinPointsCluster_)){
            this->dbMinPointsCluster_ = 18;
            cout << this->hint_ << ": No DBSCAN minimum point in each cluster parameter. Use default: 18." << endl;
        }
        else{
            cout << this->hint_ << ": DBSCAN Minimum point in each cluster is set to: " << this->dbMinPointsCluster_ << endl;
        }

        // search range
        if (not this->nh_.getParam(this->ns_ + "/dbscan_search_range_epsilon", this->dbEpsilon_)){
            this->dbEpsilon_ = 0.3;
            cout << this->hint_ << ": No DBSCAN epsilon parameter. Use default: 0.3." << endl;
        }
        else{
            cout << this->hint_ << ": DBSCAN epsilon is set to: " << this->dbEpsilon_ << endl;
        }  

        // IOU threshold
        if (not this->nh_.getParam(this->ns_ + "/filtering_BBox_IOU_threshold", this->boxIOUThresh_)){
            this->boxIOUThresh_ = 0.5;
            cout << this->hint_ << ": No threshold for boununding box IOU filtering parameter found. Use default: 0.5." << endl;
        }
        else{
            cout << this->hint_ << ": The threshold for boununding box IOU filtering is set to: " << this->boxIOUThresh_ << endl;
        }  

        // yolo thickness range threshold
        if (not this->nh_.getParam(this->ns_ + "/yolo_thickness_range_thresh", this->yoloThicknessRange_)){
            this->yoloThicknessRange_ = 1.0;
            cout << this->hint_ << ": No yolo thickness range threshold. Use default: 1.0." << endl;
        }
        else{
            cout << this->hint_ << ": The yolo thickness range threshold is set to: " << this->yoloThicknessRange_ << endl;
        }  

        // tracking history size
        if (not this->nh_.getParam(this->ns_ + "/history_size", this->histSize_)){
            this->histSize_ = 5;
            std::cout << this->hint_ << ": No tracking history isze parameter found. Use default: 5." << std::endl;
        }
        else{
            std::cout << this->hint_ << ": The history for tracking is set to: " << this->histSize_ << std::endl;
        }  

        // time difference
        if (not this->nh_.getParam(this->ns_ + "/time_difference", this->dt_)){
            this->dt_ = 0.033;
            std::cout << this->hint_ << ": No time difference parameter found. Use default: 0.033." << std::endl;
        }
        else{
            std::cout << this->hint_ << ": The time difference for the system is set to: " << this->dt_ << std::endl;
        }  


    }

    void dynamicDetector::registerPub(){
        image_transport::ImageTransport it(this->nh_);
        // uv detector depth map pub
        this->uvDepthMapPub_ = it.advertise(this->ns_ + "/detected_depth_map", 1);

        // uv detector u depth map pub
        this->uDepthMapPub_ = it.advertise(this->ns_ + "/detected_u_depth_map", 1);

        // uv detector bird view pub
        this->uvBirdViewPub_ = it.advertise(this->ns_ + "/bird_view", 1);

        // Yolo 2D bounding box on depth map pub
        this->detectedAlignedDepthImgPub_ = it.advertise(this->ns_ + "/detected_aligned_depth_map_yolo", 1);

        // uv detector bounding box pub
        this->uvBBoxesPub_ = this->nh_.advertise<visualization_msgs::MarkerArray>(this->ns_ + "/uv_bboxes", 10);

        // filtered pointcloud pub
        this->filteredPointsPub_ = this->nh_.advertise<sensor_msgs::PointCloud2>(this->ns_ + "/filtered_depth_cloud", 10);

        // DBSCAN bounding box pub
        this->dbBBoxesPub_ = this->nh_.advertise<visualization_msgs::MarkerArray>(this->ns_ + "/dbscan_bboxes", 10);

        // yolo bounding box pub
        this->yoloBBoxesPub_ = this->nh_.advertise<visualization_msgs::MarkerArray>(this->ns_ + "/yolo_3d_bboxes", 10);

        // filtered bounding box pub
        this->filteredBoxesPub_ = this->nh_.advertise<visualization_msgs::MarkerArray>(this->ns_ + "/filtered_bboxes", 10);
    }

    void dynamicDetector::registerCallback(){
        // depth pose callback
        this->depthSub_.reset(new message_filters::Subscriber<sensor_msgs::Image>(this->nh_, this->depthTopicName_, 50));
        if (this->localizationMode_ == 0){
            this->poseSub_.reset(new message_filters::Subscriber<geometry_msgs::PoseStamped>(this->nh_, this->poseTopicName_, 25));
            this->depthPoseSync_.reset(new message_filters::Synchronizer<depthPoseSync>(depthPoseSync(100), *this->depthSub_, *this->poseSub_));
            this->depthPoseSync_->registerCallback(boost::bind(&dynamicDetector::depthPoseCB, this, _1, _2));
        }
        else if (this->localizationMode_ == 1){
            this->odomSub_.reset(new message_filters::Subscriber<nav_msgs::Odometry>(this->nh_, this->odomTopicName_, 25));
            this->depthOdomSync_.reset(new message_filters::Synchronizer<depthOdomSync>(depthOdomSync(100), *this->depthSub_, *this->odomSub_));
            this->depthOdomSync_->registerCallback(boost::bind(&dynamicDetector::depthOdomCB, this, _1, _2));
        }
        else{
            ROS_ERROR("[dynamicDetector]: Invalid localization mode!");
            exit(0);
        }

        // aligned depth subscriber
        this->alignedDepthSub_ = this->nh_.subscribe(this->alignedDepthTopicName_, 10, &dynamicDetector::alignedDepthCB, this);

        // yolo detection results subscriber
        this->yoloDetectionSub_ = this->nh_.subscribe("yolo_detector/detected_bounding_boxes", 10, &dynamicDetector::yoloDetectionCB, this);

        // detection timer
        this->detectionTimer_ = this->nh_.createTimer(ros::Duration(0.033), &dynamicDetector::detectionCB, this);

        // tracking timer
        this->trackingTimer_ = this->nh_.createTimer(ros::Duration(0.033), &dynamicDetector::trackingCB, this);

        // classification timer
        this->classificationTimer_ = this->nh_.createTimer(ros::Duration(0.033), &dynamicDetector::classificationCB, this);
    
        // visualization timer
        this->visTimer_ = this->nh_.createTimer(ros::Duration(0.033), &dynamicDetector::visCB, this);
    }


    void dynamicDetector::depthPoseCB(const sensor_msgs::ImageConstPtr& img, const geometry_msgs::PoseStampedConstPtr& pose){
        // store current depth image
        cv_bridge::CvImagePtr imgPtr = cv_bridge::toCvCopy(img, img->encoding);
        if (img->encoding == sensor_msgs::image_encodings::TYPE_32FC1){
            (imgPtr->image).convertTo(imgPtr->image, CV_16UC1, this->depthScale_);
        }
        imgPtr->image.copyTo(this->depthImage_);

        // store current position and orientation (camera)
        Eigen::Matrix4d camPoseMatrix, camPoseColorMatrix;
        this->getCameraPose(pose, camPoseMatrix, camPoseColorMatrix);

        this->position_(0) = camPoseMatrix(0, 3);
        this->position_(1) = camPoseMatrix(1, 3);
        this->position_(2) = camPoseMatrix(2, 3);
        this->orientation_ = camPoseMatrix.block<3, 3>(0, 0);

        this->positionColor_(0) = camPoseColorMatrix(0, 3);
        this->positionColor_(1) = camPoseColorMatrix(1, 3);
        this->positionColor_(2) = camPoseColorMatrix(2, 3);
        this->orientationColor_ = camPoseColorMatrix.block<3, 3>(0, 0);
    }

    void dynamicDetector::depthOdomCB(const sensor_msgs::ImageConstPtr& img, const nav_msgs::OdometryConstPtr& odom){
        // store current depth image
        cv_bridge::CvImagePtr imgPtr = cv_bridge::toCvCopy(img, img->encoding);
        if (img->encoding == sensor_msgs::image_encodings::TYPE_32FC1){
            (imgPtr->image).convertTo(imgPtr->image, CV_16UC1, this->depthScale_);
        }
        imgPtr->image.copyTo(this->depthImage_);

        // store current position and orientation (camera)
        Eigen::Matrix4d camPoseMatrix, camPoseColorMatrix;
        this->getCameraPose(odom, camPoseMatrix, camPoseColorMatrix);

        this->position_(0) = camPoseMatrix(0, 3);
        this->position_(1) = camPoseMatrix(1, 3);
        this->position_(2) = camPoseMatrix(2, 3);
        this->orientation_ = camPoseMatrix.block<3, 3>(0, 0);

        this->positionColor_(0) = camPoseColorMatrix(0, 3);
        this->positionColor_(1) = camPoseColorMatrix(1, 3);
        this->positionColor_(2) = camPoseColorMatrix(2, 3);
        this->orientationColor_ = camPoseColorMatrix.block<3, 3>(0, 0);
    }

    void dynamicDetector::alignedDepthCB(const sensor_msgs::ImageConstPtr& img){
        cv_bridge::CvImagePtr imgPtr = cv_bridge::toCvCopy(img, img->encoding);
        if (img->encoding == sensor_msgs::image_encodings::TYPE_32FC1){
            (imgPtr->image).convertTo(imgPtr->image, CV_16UC1, this->depthScale_);
        }
        imgPtr->image.copyTo(this->alignedDepthImage_);

        cv::Mat depthNormalized;
        imgPtr->image.copyTo(depthNormalized);
        double min, max;
        cv::minMaxIdx(depthNormalized, &min, &max);
        cv::convertScaleAbs(depthNormalized, depthNormalized, 255. / max);
        depthNormalized.convertTo(depthNormalized, CV_8UC1);
        cv::applyColorMap(depthNormalized, depthNormalized, cv::COLORMAP_BONE);
        this->detectedAlignedDepthImg_ = depthNormalized;
    }

    void dynamicDetector::yoloDetectionCB(const vision_msgs::Detection2DArrayConstPtr& detections){
        this->yoloDetectionResults_ = *detections;
    }

    void dynamicDetector::detectionCB(const ros::TimerEvent&){
        cout << "detector CB" << endl;
        ros::Time dbStartTime = ros::Time::now();
        this->dbscanDetect();
        ros::Time dbEndTime = ros::Time::now();
        cout << "dbscan detect time: " << (dbEndTime - dbStartTime).toSec() << endl;

        ros::Time uvStartTime = ros::Time::now();
        this->uvDetect();
        ros::Time uvEndTime = ros::Time::now();
        cout << "uv detect time: " << (uvEndTime - uvStartTime).toSec() << endl;


        this->yoloDetectionTo3D();

        this->filterBBoxes();

        this->newDetectFlag_ = true; // get a new detection
    }

    void dynamicDetector::trackingCB(const ros::TimerEvent&){
        cout << "tracking CB Not implemented yet." << endl;
        // data association
        this->boxAssociation();

        // kalman filter tracking

    }

    void dynamicDetector::classificationCB(const ros::TimerEvent&){
        cout << "classification CB not implemented yet." << endl;
    }

    void dynamicDetector::visCB(const ros::TimerEvent&){
        this->publishUVImages();
        this->publish3dBox(this->uvBBoxes_, this->uvBBoxesPub_, 0, 1, 0);
        this->publishPoints(this->filteredPoints_, this->filteredPointsPub_);
        this->publish3dBox(this->dbBBoxes_, this->dbBBoxesPub_, 1, 0, 0);
        this->publishYoloImages();
        this->publish3dBox(this->yoloBBoxes_, this->yoloBBoxesPub_, 1, 0, 1);

        this->publish3dBox(this->filteredBBoxes_, this->filteredBoxesPub_, 0, 0, 1);
    }

    void dynamicDetector::uvDetect(){
        // initialization
        if (this->uvDetector_ == NULL){
            this->uvDetector_.reset(new UVdetector ());
            this->uvDetector_->fx = this->fx_;
            this->uvDetector_->fy = this->fy_;
            this->uvDetector_->px = this->cx_;
            this->uvDetector_->py = this->cy_;
            this->uvDetector_->depthScale_ = this->depthScale_; 
            this->uvDetector_->max_dist = this->raycastMaxLength_ * 1000;
        }

        // detect from depth mapcalBox
        if (not this->depthImage_.empty()){
            this->uvDetector_->depth = this->depthImage_;
            this->uvDetector_->detect();
            this->uvDetector_->extract_3Dbox();

            this->uvDetector_->display_U_map();
            this->uvDetector_->display_bird_view();
            this->uvDetector_->display_depth();


            // transform to the world frame (recalculate the boudning boxes)
            std::vector<mapManager::box3D> uvBBoxes;
            this->transformUVBBoxes(uvBBoxes);
            this->uvBBoxes_ = uvBBoxes;
        }
    }

    void dynamicDetector::dbscanDetect(){
        // 1. get pointcloud
        this->projectDepthImage();

        // 2. filter points
        this->filterPoints(this->projPoints_, this->filteredPoints_);

        // 3. cluster points and get bounding boxes
        this->clusterPointsAndBBoxes(this->filteredPoints_, this->dbBBoxes_, this->pcClusters_);
    }

    void dynamicDetector::yoloDetectionTo3D(){
        std::vector<mapManager::box3D> yoloBBoxesTemp;
        for (size_t i=0; i<this->yoloDetectionResults_.detections.size(); ++i){
            mapManager::box3D bbox3D;
            cv::Rect bboxVis;
            this->getYolo3DBBox(this->yoloDetectionResults_.detections[i], bbox3D, bboxVis);
            cv::rectangle(this->detectedAlignedDepthImg_, bboxVis, cv::Scalar(0, 255, 0), 5, 8, 0);
            yoloBBoxesTemp.push_back(bbox3D);
        }
        this->yoloBBoxes_ = yoloBBoxesTemp;    
    }

    void dynamicDetector::transformUVBBoxes(std::vector<mapManager::box3D>& bboxes){
        bboxes.clear();
        for(size_t i = 0; i < this->uvDetector_->box3Ds.size(); ++i){
            mapManager::box3D bbox;
            double x = this->uvDetector_->box3Ds[i].x; 
            double y = this->uvDetector_->box3Ds[i].y;
            double z = this->uvDetector_->box3Ds[i].z;
            double xWidth = this->uvDetector_->box3Ds[i].x_width;
            double yWidth = this->uvDetector_->box3Ds[i].y_width;
            double zWidth = this->uvDetector_->box3Ds[i].z_width;

            Eigen::Vector3d center (x, y, z);
            Eigen::Vector3d size (xWidth, yWidth, zWidth);
            Eigen::Vector3d newCenter, newSize;

            this->transformBBox(center, size, this->position_, this->orientation_, newCenter, newSize);

            // assign values to bounding boxes in the map frame
            bbox.x = newCenter(0);
            bbox.y = newCenter(1);
            bbox.z = newCenter(2);
            bbox.x_width = newSize(0);
            bbox.y_width = newSize(1);
            bbox.z_width = newSize(2);
            bboxes.push_back(bbox);            
        }        
    }


    void dynamicDetector::filterBBoxes(){
        std::vector<mapManager::box3D> filteredBBoxesTemp;
        for (size_t i=0 ; i<this->uvBBoxes_.size() ; i++){
            for (size_t j=0 ; j<this->dbBBoxes_.size() ; j++){
                
                float IOU = this->calBoxIOU(this->uvBBoxes_[i], this->dbBBoxes_[j]);
                if (IOU > this->boxIOUThresh_){
                    
                    mapManager::box3D box;
                    
                    // take concervative strategy
                    float xmax = std::max(this->uvBBoxes_[i].x+this->uvBBoxes_[i].x_width/2, this->dbBBoxes_[j].x+this->dbBBoxes_[j].x_width/2);
                    float xmin = std::min(this->uvBBoxes_[i].x-this->uvBBoxes_[i].x_width/2, this->dbBBoxes_[j].x-this->dbBBoxes_[j].x_width/2);
                    float ymax = std::max(this->uvBBoxes_[i].y+this->uvBBoxes_[i].y_width/2, this->dbBBoxes_[j].y+this->dbBBoxes_[j].y_width/2);
                    float ymin = std::min(this->uvBBoxes_[i].y-this->uvBBoxes_[i].y_width/2, this->dbBBoxes_[j].y-this->dbBBoxes_[j].y_width/2);
                    float zmax = std::max(this->uvBBoxes_[i].z+this->uvBBoxes_[i].z_width/2, this->dbBBoxes_[j].z+this->dbBBoxes_[j].z_width/2);
                    float zmin = std::min(this->uvBBoxes_[i].z-this->uvBBoxes_[i].z_width/2, this->dbBBoxes_[j].z-this->dbBBoxes_[j].z_width/2);
                    box.x = (xmin+xmax)/2;
                    box.y = (ymin+ymax)/2;
                    box.z = (zmin+zmax)/2;
                    box.x_width = xmax-xmin;
                    box.y_width = ymax-ymin;
                    box.z_width = zmax-zmin;

                    // take average
                    // box.x = (this->uvBBoxes_[i].x+this->dbBBoxes_[j].x)/2;
                    // box.y = (this->uvBBoxes_[i].y+this->dbBBoxes_[j].y)/2;
                    // box.z = (this->uvBBoxes_[i].z+this->dbBBoxes_[j].z)/2;
                    // box.x_width = (this->uvBBoxes_[i].x_width+this->dbBBoxes_[j].x_width)/2;
                    // box.y_width = (this->uvBBoxes_[i].y_width+this->dbBBoxes_[j].y_width)/2;
                    // box.z_width = (this->uvBBoxes_[i].z_width+this->dbBBoxes_[j].z_width)/2;
                    ROS_INFO("in loop");
                    this->filteredBBoxes_.push_back(box);
                    cout << "dbBBox size " << this->dbBBoxes_[j].x<< endl;
                    cout << "pccluster size " << this->pcClusters_.size() << " j " << j << endl;
                    this->filteredPcClusters_.push_back(this->pcClusters_[j]);
                    ROS_INFO("one loop end");
                    filteredBBoxesTemp.push_back(box);

                }
            }
        }
        this->filteredBBoxes_ = filteredBBoxesTemp;
        ROS_INFO("size for both: %lu, %lu",this->filteredBBoxes_.size(), this->filteredPcClusters_.size());
    }

    void dynamicDetector::boxAssociation(){
        // init history for the first frame
        if (!this->boxHist_.size()){
            this->boxHist_.resize(this->filteredBBoxes_.size());
            ROS_INFO(" first frame, association not started yet. filtered boxes size: %lu", this->filteredBBoxes_.size());
        }
        else{
            // start association only if a new detection is available
            if (this->newDetectFlag_){

                std::vector<mapManager::box3D> propedBoxes;
                // Eigen::MatrixXd propedBoxesFeat(propedBoxes.size(), 6);
                // Eigen::MatrixXd currBoxesFeat(this->filteredBBoxes_.size(),6);
                // Eigen::MatrixXd similarity;
                
                // linear propagation
                mapManager::box3D propedBox;
                for (size_t i=0 ; i<this->boxHist_.size() ; i++){
                    propedBox = this->boxHist_[i][0];
                    propedBox.x = propedBox.Vx*this->dt_;
                    propedBox.y = propedBox.Vy*this->dt_;
                    propedBoxes.push_back(propedBox);
                }


                Eigen::MatrixXd propedBoxesFeat(propedBoxes.size(), 6);
                Eigen::MatrixXd currBoxesFeat(this->filteredBBoxes_.size(),6);

                Eigen::MatrixXd similarity;

                // generate feature
                this->genFeatHelper(propedBoxesFeat, propedBoxes);
                this->genFeatHelper(currBoxesFeat, this->filteredBBoxes_);

                currBoxesFeat.transposeInPlace();

                // propedBoxesFeat.resize(propedBoxes.size());
                // for (size_t i=0 ; i<propedBoxes.size() ; i++){
                //     propedBoxesFeat(i)(0) = propedBoxes[i].x;
                //     propedBoxesFeat(i)(1) = propedBoxes[i].y;
                //     propedBoxesFeat(i)(2) = propedBoxes[i].z;
                //     propedBoxesFeat(i)(3) = propedBoxes[i].x_width;
                //     propedBoxesFeat(i)(4) = propedBoxes[i].y_width;
                //     propedBoxesFeat(i)(5) = propedBoxes[i].z_width;
                // }
                // currBoxesFeat.resize(this->filteredBoxes.size());
                // for (size_t i=0 ; i<this->filteredBoxes.size() ; i++){
                //     propedBoxesFeat(0)(i) = this->filteredBoxes[i].x;
                //     propedBoxesFeat(i)(1) = this->filteredBoxes[i].y;
                //     propedBoxesFeat(i)(2) = this->filteredBoxes[i].z;
                //     propedBoxesFeat(i)(3) = this->filteredBoxes[i].x_width;
                //     propedBoxesFeat(i)(4) = this->filteredBoxes[i].y_width;
                //     propedBoxesFeat(i)(5) = this->filteredBoxes[i].z_width;
                // }

                // calculate association
                cout<< "propedBoxesFeat "<< propedBoxesFeat << endl;
                cout<< "rowwise norm "<< propedBoxesFeat.rowwise().norm() << endl;
                cout << "norm" << currBoxesFeat.rowwise().norm() << endl;
                // Eigen::VectorXd unitPropedBoxesFeat = propedBoxesFeat.rowwise()/propedBoxesFeat.rowwise().norm();
                // Eigen::MatrixXd unitCurrBoxesFeat = currBoxesFeat.rowwise()/currBoxesFeat.rowwise().norm();

                // cout << "unit proposed feat "<<unitPropedBoxesFeat<<endl;
                // cout << "unit current feat "<<unitCurrBoxesFeat<<endl;

                // similarity = unitPropedBoxesFeat * unitCurrBoxesFeat;

                // similarity = (unitPropedBoxesFeat * currBoxesFeat)/(currBoxesFeat.rowwise().norm());

                // similarity = (propedBoxesFeat.cwiseQuotient() * currBoxesFeat)/();
                cout << "similarity "<<similarity<<endl;

                // update history
                // this->boxHist_.push_front(this->filteredBBoxes_[]);
            }
            else {
                ROS_INFO("new detect not comming");
            }

        }

        this->newDetectFlag_ = false; // the most recent detection has been associated
    }


    void dynamicDetector::genFeatHelper(Eigen::MatrixXd& feature, const std::vector<mapManager::box3D>& boxes){ 
        for (size_t i=0 ; i<boxes.size() ; i++){
            feature(i,0) = boxes[i].x;
            feature(i,1) = boxes[i].y;
            feature(i,2) = boxes[i].z;
            feature(i,3) = boxes[i].x_width;
            feature(i,4) = boxes[i].y_width;
            feature(i,5) = boxes[i].z_width;
        }
    }

    void dynamicDetector::projectDepthImage(){
        this->projPointsNum_ = 0;

        int cols = this->depthImage_.cols;
        int rows = this->depthImage_.rows;
        uint16_t* rowPtr;

        Eigen::Vector3d currPointCam, currPointMap;
        double depth;
        const double inv_factor = 1.0 / this->depthScale_;
        const double inv_fx = 1.0 / this->fx_;
        const double inv_fy = 1.0 / this->fy_;

        // iterate through each pixel in the depth image
        for (int v=this->depthFilterMargin_; v<rows-this->depthFilterMargin_; v=v+this->skipPixel_){ // row
            rowPtr = this->depthImage_.ptr<uint16_t>(v) + this->depthFilterMargin_;
            for (int u=this->depthFilterMargin_; u<cols-this->depthFilterMargin_; u=u+this->skipPixel_){ // column
                depth = (*rowPtr) * inv_factor;
                
                if (*rowPtr == 0) {
                    depth = this->raycastMaxLength_ + 0.1;
                } else if (depth < this->depthMinValue_) {
                    continue;
                } else if (depth > this->depthMaxValue_) {
                    depth = this->raycastMaxLength_ + 0.1;
                }
                rowPtr =  rowPtr + this->skipPixel_;

                // get 3D point in camera frame
                currPointCam(0) = (u - this->cx_) * depth * inv_fx;
                currPointCam(1) = (v - this->cy_) * depth * inv_fy;
                currPointCam(2) = depth;
                currPointMap = this->orientation_ * currPointCam + this->position_; // transform to map coordinate

                this->projPoints_[this->projPointsNum_] = currPointMap;
                this->pointsDepth_[this->projPointsNum_] = depth;
                this->projPointsNum_ = this->projPointsNum_ + 1;
            }
        } 
    }

    void dynamicDetector::filterPoints(const std::vector<Eigen::Vector3d>& points, std::vector<Eigen::Vector3d>& filteredPoints){
        // currently there is only one filtered (might include more in the future)
        std::vector<Eigen::Vector3d> voxelFilteredPoints;
        this->voxelFilter(points, voxelFilteredPoints);
        filteredPoints = voxelFilteredPoints;
    }


    void dynamicDetector::clusterPointsAndBBoxes(const std::vector<Eigen::Vector3d>& points, std::vector<mapManager::box3D>& bboxes, std::vector<std::vector<Eigen::Vector3d>>& pcClusters){
        std::vector<mapManager::Point> pointsDB;
        this->eigenToDBPointVec(points, pointsDB, points.size());

        this->dbCluster_.reset(new DBSCAN (this->dbMinPointsCluster_, this->dbEpsilon_, pointsDB));

        // DBSCAN clustering
        this->dbCluster_->run();

        // get the cluster data with bounding boxes
        // iterate through all the clustered points and find number of clusters
        int clusterNum = 0;
        for (size_t i=0; i<this->dbCluster_->m_points.size(); ++i){
            mapManager::Point pDB = this->dbCluster_->m_points[i];
            if (pDB.clusterID > clusterNum){
                clusterNum = pDB.clusterID;
            }
        }

        pcClusters.clear();
        pcClusters.resize(clusterNum);
        for (size_t i=0; i<this->dbCluster_->m_points.size(); ++i){
            mapManager::Point pDB = this->dbCluster_->m_points[i];
            if (pDB.clusterID > 0){
                Eigen::Vector3d p = this->dbPointToEigen(pDB);
                pcClusters[pDB.clusterID-1].push_back(p);
            }            
        }

        // calculate the bounding boxes based on the clusters
        bboxes.clear();
        // bboxes.resize(clusterNum);
        for (size_t i=0; i<pcClusters.size(); ++i){
            mapManager::box3D box;

            double xmin = pcClusters[i][0](0);
            double ymin = pcClusters[i][0](1);
            double zmin = pcClusters[i][0](2);
            double xmax = pcClusters[i][0](0);
            double ymax = pcClusters[i][0](1);
            double zmax = pcClusters[i][0](2);
            for (size_t j=0; j<pcClusters[i].size(); ++j){
                xmin = (pcClusters[i][j](0)<xmin)?pcClusters[i][j](0):xmin;
                ymin = (pcClusters[i][j](1)<ymin)?pcClusters[i][j](1):ymin;
                zmin = (pcClusters[i][j](2)<zmin)?pcClusters[i][j](2):zmin;
                xmax = (pcClusters[i][j](0)>xmax)?pcClusters[i][j](0):xmax;
                ymax = (pcClusters[i][j](1)>ymax)?pcClusters[i][j](1):ymax;
                zmax = (pcClusters[i][j](2)>zmax)?pcClusters[i][j](2):zmax;
            }
            box.id = i;

            box.x = (xmax + xmin)/2.0;
            box.y = (ymax + ymin)/2.0;
            box.z = (zmax + zmin)/2.0;
            box.x_width = (xmax - xmin);
            box.y_width = (ymax - ymin);
            box.z_width = (zmax - zmin);
            bboxes.push_back(box);
        }


    }

    void dynamicDetector::voxelFilter(const std::vector<Eigen::Vector3d>& points, std::vector<Eigen::Vector3d>& filteredPoints){
        const double res = 0.1; // resolution of voxel
        int xVoxels = ceil(this->localSensorRange_(0)/res); int yVoxels = ceil(this->localSensorRange_(1)/res); int zVoxels = ceil(this->localSensorRange_(2)/res);
        int totalVoxels = xVoxels * yVoxels * zVoxels;
        // std::vector<bool> voxelOccupancyVec (totalVoxels, false);
        std::vector<int> voxelOccupancyVec (totalVoxels, 0);

        // Iterate through each points in the cloud
        filteredPoints.clear();
        for (int i=0; i<this->projPointsNum_; ++i){
            Eigen::Vector3d p = points[i];

            if (this->isInFilterRange(p) and p(2) >= this->groundHeight_ and this->pointsDepth_[i] <= this->raycastMaxLength_){
                // find the corresponding voxel id in the vector and check whether it is occupied
                int pID = this->posToAddress(p, res);

                // add one point
                voxelOccupancyVec[pID] +=1;

                // if occupied, skip. Else add to the filtered points
                // if (voxelOccupancyVec[pID] == true){
                //     continue;
                // }
                // else{
                //     voxelOccupancyVec[pID] = true;
                //     filteredPoints.push_back(p);
                // }
                // add only if 5 points are found
                if (voxelOccupancyVec[pID] == 10){
                    filteredPoints.push_back(p);
                }
            }
        }  
    }

    float dynamicDetector::calBoxIOU(mapManager::box3D& box1, mapManager::box3D& box2){
        float box1Volume = box1.x_width * box1.y_width * box1.z_width;
        float box2Volume = box2.x_width * box2.y_width * box2.z_width;

        float l1Y = box1.y+box1.y_width/2-(box2.y-box2.y_width/2);
        float l2Y = box2.y+box2.y_width/2-(box1.y-box1.y_width/2);
        float l1X = box1.x+box1.x_width/2-(box2.x-box2.x_width/2);
        float l2X = box2.x+box2.x_width/2-(box1.x-box1.x_width/2);
        float l1Z = box1.z+box1.z_width/2-(box2.z-box2.z_width/2);
        float l2Z = box2.z+box2.z_width/2-(box1.z-box1.z_width/2);
        float overlapX = std::min( l1X , l2X );
        float overlapY = std::min( l1Y , l2Y );
        float overlapZ = std::min( l1Z , l2Z );
        cout << box1.z <<" " << box1.z_width << " " <<box2.z << " " << box2.z_width << endl;
        cout << l1Z <<" "<<l2Z <<endl;
        cout << "ovelap xyz" << overlapX << " " << overlapY << " " <<overlapZ << endl;
        // include: C-IOU
        if (std::max(l1X, l2X)<=std::max(box1.x_width,box2.x_width)){ 
            overlapX = std::min(box1.x_width, box2.x_width);
        }
        if (std::max(l1Y, l2Y)<=std::max(box1.y_width,box2.y_width)){ 
            overlapY = std::min(box1.y_width, box2.y_width);
        }
        if (std::max(l1Z, l2Z)<=std::max(box1.z_width,box2.z_width)){ 
            overlapZ = std::min(box1.z_width, box2.z_width);
        }
        // overlapX = overlapLengthIfCIOU(overlapX, l1X, l2X, box1.x_width, box2.x_width);
        // overlapY = overlapLengthIfCIOU(overlapY, l1Y, l2Y, box1.y+width, box2.y_width);
        // overlapZ = overlapLengthIfCIOU(overlapZ, l1Z, l2Z, box1.z_width, box2.z_width);


        float overlapVolume = overlapX * overlapY *  overlapZ;
        float IOU = overlapVolume / (box1Volume+box2Volume-overlapVolume);
        cout << "overlap volume " << overlapVolume <<endl;
        
        // D-IOU
        if (overlapX<=0 || overlapY<=0 ||overlapZ<=0){
            IOU = 0;
        }
        return IOU;
    }

    void dynamicDetector::getYolo3DBBox(const vision_msgs::Detection2D& detection, mapManager::box3D& bbox3D, cv::Rect& bboxVis){
        if (this->alignedDepthImage_.empty()){
            return;
        }        
        // 1. retrive 2D detection result
        int topX = int(detection.bbox.center.x); 
        int topY = int(detection.bbox.center.y); 
        int xWidth = int(detection.bbox.size_x); 
        int yWidth = int(detection.bbox.size_y); 
        bboxVis.x = topX;
        bboxVis.y = topY;
        bboxVis.height = yWidth;
        bboxVis.width = xWidth;

        // 2. get thickness estimation (MAD: Median Absolute Deviation)
        uint16_t* rowPtr;
        double depth;
        const double inv_factor = 1.0 / this->depthScale_;
        int vMin = std::min(topY, this->depthFilterMargin_);
        int uMin = std::min(topX, this->depthFilterMargin_);
        int vMax = std::min(topY+yWidth, this->imgRows_-this->depthFilterMargin_);
        int uMax = std::min(topX+xWidth, this->imgCols_-this->depthFilterMargin_);
        std::vector<double> depthValues;


        // find median depth value
        for (int v=vMin; v<vMax; ++v){ // row
            rowPtr = this->alignedDepthImage_.ptr<uint16_t>(v);
            for (int u=uMin; u<uMax; ++u){ // column
                depth = (*rowPtr) * inv_factor;
                if (depth >= this->depthMinValue_ and depth <= this->depthMaxValue_){
                    depthValues.push_back(depth);
                }
                ++rowPtr;
            }
        }

        if (depthValues.size() == 0){ // in case of out of range
            return;
        }
        std::sort(depthValues.begin(), depthValues.end());
        double depthMedian = depthValues[int(depthValues.size()/2)];


        // find median absolute deviation
        std::vector<double> depthMedianDeviation;
        for (int v=vMin; v<vMax; ++v){ // row
            rowPtr = this->alignedDepthImage_.ptr<uint16_t>(v);
            for (int u=uMin; u<uMax; ++u){ // column
                depth = (*rowPtr) * inv_factor;
                if (depth >= this->depthMinValue_ and depth <= this->depthMaxValue_){
                    depthMedianDeviation.push_back(std::abs(depth - depthMedian));
                }
                ++rowPtr;
            }
        }        
        std::sort(depthMedianDeviation.begin(), depthMedianDeviation.end());
        double MAD = depthMedianDeviation[int(depthMedianDeviation.size()/2)];

        double depthMin = 10.0; double depthMax = -10.0;
        // find min max depth value
        for (int v=vMin; v<vMax; ++v){ // row
            rowPtr = this->alignedDepthImage_.ptr<uint16_t>(v);
            for (int u=uMin; u<uMax; ++u){ // column
                depth = (*rowPtr) * inv_factor;
                if (depth >= this->depthMinValue_ and depth <= this->depthMaxValue_){
                    if (depth < depthMin and depth >= depthMedian - 1.5 * MAD){
                        depthMin = depth;
                    }

                    if (depth > depthMax and depth <= depthMedian + 1.5 * MAD){
                        depthMax = depth;
                    }
                }
                ++rowPtr;
            }
        }
        
        if (depthMin == 10.0 or depthMax == -10.0){ // in case the depth value is not available
            return;
        }

        // 3. project points into 3D in the camera frame
        Eigen::Vector3d pUL, pBR, center;
        pUL(0) = (topX - this->cxC_) * (depthMin + depthMax) / 2.0 / this->fxC_;
        pUL(1) = (topY - this->cyC_) * (depthMin + depthMax) / 2.0 / this->fyC_;
        pUL(2) = (depthMin + depthMax) / 2.0;

        pBR(0) = (topX + xWidth - this->cxC_) * (depthMin + depthMax) / 2.0 / this->fxC_;
        pBR(1) = (topY + yWidth- this->cyC_) * (depthMin + depthMax) / 2.0 / this->fyC_;
        pBR(2) = (depthMin + depthMax) / 2.0;

        center(0) = (pUL(0) + pBR(0))/2.0;
        center(1) = (pUL(1) + pBR(1))/2.0;
        center(2) = (depthMin + depthMax)/2.0;

        double xWidth3D = std::abs(pBR(0) - pUL(0));
        double yWidth3D = std::abs(pBR(1) - pUL(1));
        double zWidth3D = depthMax - depthMin;        
        Eigen::Vector3d size (xWidth3D, yWidth3D, zWidth3D);

        // 4. transform 3D points into world frame
        Eigen::Vector3d newCenter, newSize;
        this->transformBBox(center, size, this->positionColor_, this->orientationColor_, newCenter, newSize);
        bbox3D.x = newCenter(0);
        bbox3D.y = newCenter(1);
        bbox3D.z = newCenter(2);

        bbox3D.x_width = newSize(0);
        bbox3D.y_width = newSize(1);
        bbox3D.z_width = newSize(2);
    }

    void dynamicDetector::publishUVImages(){
        sensor_msgs::ImagePtr depthBoxMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", this->uvDetector_->depth_show).toImageMsg();
        sensor_msgs::ImagePtr UmapBoxMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", this->uvDetector_->U_map_show).toImageMsg();
        sensor_msgs::ImagePtr birdBoxMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", this->uvDetector_->bird_view).toImageMsg();  
        this->uvDepthMapPub_.publish(depthBoxMsg);
        this->uDepthMapPub_.publish(UmapBoxMsg); 
        this->uvBirdViewPub_.publish(birdBoxMsg);     
    }

    void dynamicDetector::publishYoloImages(){
        sensor_msgs::ImagePtr detectedAlignedImgMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", this->detectedAlignedDepthImg_).toImageMsg();
        this->detectedAlignedDepthImgPub_.publish(detectedAlignedImgMsg);
    }

    void dynamicDetector::publishPoints(const std::vector<Eigen::Vector3d>& points, const ros::Publisher& publisher){
        pcl::PointXYZ pt;
        pcl::PointCloud<pcl::PointXYZ> cloud;        
        for (size_t i=0; i<points.size(); ++i){
            pt.x = points[i](0);
            pt.y = points[i](1);
            pt.z = points[i](2);
            cloud.push_back(pt);
        }    
        cloud.width = cloud.points.size();
        cloud.height = 1;
        cloud.is_dense = true;
        cloud.header.frame_id = "map";

        sensor_msgs::PointCloud2 cloudMsg;
        pcl::toROSMsg(cloud, cloudMsg);
        publisher.publish(cloudMsg);
    }


    void dynamicDetector::publish3dBox(const std::vector<box3D>& boxes, const ros::Publisher& publisher, double r, double g, double b) {
        // visualization using bounding boxes 
        visualization_msgs::Marker line;
        visualization_msgs::MarkerArray lines;
        line.header.frame_id = "map";
        line.type = visualization_msgs::Marker::LINE_LIST;
        line.action = visualization_msgs::Marker::ADD;
        line.ns = "box3D";  
        line.scale.x = 0.1;
        line.color.r = r;
        line.color.g = g;
        line.color.b = b;
        line.color.a = 1.0;
        line.lifetime = ros::Duration(0.1);

        for(size_t i = 0; i < boxes.size(); i++){
            // visualization msgs

            double x = boxes[i].x; 
            double y = boxes[i].y; 
            double z = boxes[i].z; 

            // double x_width = std::max(boxes[i].x_width,boxes[i].y_width);
            // double y_width = std::max(boxes[i].x_width,boxes[i].y_width);
            double x_width = boxes[i].x_width;
            double y_width = boxes[i].y_width;
            double z_width = boxes[i].z_width;
            
            vector<geometry_msgs::Point> verts;
            geometry_msgs::Point p;
            // vertice 0
            p.x = x-x_width / 2.; p.y = y-y_width / 2.; p.z = z-z_width / 2.;
            verts.push_back(p);

            // vertice 1
            p.x = x-x_width / 2.; p.y = y+y_width / 2.; p.z = z-z_width / 2.;
            verts.push_back(p);

            // vertice 2
            p.x = x+x_width / 2.; p.y = y+y_width / 2.; p.z = z-z_width / 2.;
            verts.push_back(p);

            // vertice 3
            p.x = x+x_width / 2.; p.y = y-y_width / 2.; p.z = z-z_width / 2.;
            verts.push_back(p);

            // vertice 4
            p.x = x-x_width / 2.; p.y = y-y_width / 2.; p.z = z+z_width / 2.;
            verts.push_back(p);

            // vertice 5
            p.x = x-x_width / 2.; p.y = y+y_width / 2.; p.z = z+z_width / 2.;
            verts.push_back(p);

            // vertice 6
            p.x = x+x_width / 2.; p.y = y+y_width / 2.; p.z = z+z_width / 2.;
            verts.push_back(p);

            // vertice 7
            p.x = x+x_width / 2.; p.y = y-y_width / 2.; p.z = z+z_width / 2.;
            verts.push_back(p);
            
            int vert_idx[12][2] = {
                {0,1},
                {1,2},
                {2,3},
                {0,3},
                {0,4},
                {1,5},
                {3,7},
                {2,6},
                {4,5},
                {5,6},
                {4,7},
                {6,7}
            };
            
            for (size_t i=0;i<12;i++){
                line.points.push_back(verts[vert_idx[i][0]]);
                line.points.push_back(verts[vert_idx[i][1]]);
            }
            
            lines.markers.push_back(line);
            
            line.id++;
        }
        // publish
        publisher.publish(lines);
    }

    void dynamicDetector::transformBBox(const Eigen::Vector3d& center, const Eigen::Vector3d& size, const Eigen::Vector3d& position, const Eigen::Matrix3d& orientation,
                                               Eigen::Vector3d& newCenter, Eigen::Vector3d& newSize){
        double x = center(0); 
        double y = center(1);
        double z = center(2);
        double xWidth = size(0);
        double yWidth = size(1);
        double zWidth = size(2);

        // get 8 bouding boxes coordinates in the camera frame
        Eigen::Vector3d p1 (x+xWidth/2.0, y+yWidth/2.0, z+zWidth/2.0);
        Eigen::Vector3d p2 (x+xWidth/2.0, y+yWidth/2.0, z-zWidth/2.0);
        Eigen::Vector3d p3 (x+xWidth/2.0, y-yWidth/2.0, z+zWidth/2.0);
        Eigen::Vector3d p4 (x+xWidth/2.0, y-yWidth/2.0, z-zWidth/2.0);
        Eigen::Vector3d p5 (x-xWidth/2.0, y+yWidth/2.0, z+zWidth/2.0);
        Eigen::Vector3d p6 (x-xWidth/2.0, y+yWidth/2.0, z-zWidth/2.0);
        Eigen::Vector3d p7 (x-xWidth/2.0, y-yWidth/2.0, z+zWidth/2.0);
        Eigen::Vector3d p8 (x-xWidth/2.0, y-yWidth/2.0, z-zWidth/2.0);

        // transform 8 points to the map coordinate frame
        Eigen::Vector3d p1m = orientation * p1 + position;
        Eigen::Vector3d p2m = orientation * p2 + position;
        Eigen::Vector3d p3m = orientation * p3 + position;
        Eigen::Vector3d p4m = orientation * p4 + position;
        Eigen::Vector3d p5m = orientation * p5 + position;
        Eigen::Vector3d p6m = orientation * p6 + position;
        Eigen::Vector3d p7m = orientation * p7 + position;
        Eigen::Vector3d p8m = orientation * p8 + position;
        std::vector<Eigen::Vector3d> pointsMap {p1m, p2m, p3m, p4m, p5m, p6m, p7m, p8m};

        // find max min in x, y, z directions
        double xmin=p1m(0); double xmax=p1m(0); 
        double ymin=p1m(1); double ymax=p1m(1);
        double zmin=p1m(2); double zmax=p1m(2);
        for (Eigen::Vector3d pm : pointsMap){
            if (pm(0) < xmin){xmin = pm(0);}
            if (pm(0) > xmax){xmax = pm(0);}
            if (pm(1) < ymin){ymin = pm(1);}
            if (pm(1) > ymax){ymax = pm(1);}
            if (pm(2) < zmin){zmin = pm(2);}
            if (pm(2) > zmax){zmax = pm(2);}
        }
        newCenter(0) = (xmin + xmax)/2.0;
        newCenter(1) = (ymin + ymax)/2.0;
        newCenter(2) = (zmin + zmax)/2.0;
        newSize(0) = xmax - xmin;
        newSize(1) = ymax - ymin;
        newSize(2) = zmax - zmin;
    }
}

