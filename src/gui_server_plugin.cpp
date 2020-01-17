#include "gui_server_plugin.h"

#include <ros/node_handle.h>
#include <ros/advertise_service_options.h>

#include <ed/world_model.h>
#include <ed/entity.h>
#include <ed/measurement.h>
#include <ed/io/filesystem/write.h>
#include <ed/error_context.h>

#include <rgbd/Image.h>

#include <geolib/Shape.h>
#include <geolib/ros/msg_conversions.h>

#include <tue/config/configuration.h>
#include <tue/config/loaders/yaml.h>
#include <tue/config/reader.h>

#include <ed_gui_server/EntityInfos.h>
#include <ed_gui_server/objPosVel.h>
#include <ed_gui_server/objsPosVel.h>
#include <ed/serialization/serialization.h>

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <boost/filesystem.hpp>

#include <opencv2/highgui/highgui.hpp>

float COLORS[27][3] = { { 0.6, 0.6, 0.6},
                        { 0.6, 0.6, 0.4},
                        { 0.6, 0.6, 0.2},
                        { 0.6, 0.4, 0.6},
                        { 0.6, 0.4, 0.4},
                        { 0.6, 0.4, 0.2},
                        { 0.6, 0.2, 0.6},
                        { 0.6, 0.2, 0.4},
                        { 0.6, 0.2, 0.2},
                        { 0.4, 0.6, 0.6},
                        { 0.4, 0.6, 0.4},
                        { 0.4, 0.6, 0.2},
                        { 0.4, 0.4, 0.6},
                        { 0.4, 0.4, 0.4},
                        { 0.4, 0.4, 0.2},
                        { 0.4, 0.2, 0.6},
                        { 0.4, 0.2, 0.4},
                        { 0.4, 0.2, 0.2},
                        { 0.2, 0.6, 0.6},
                        { 0.2, 0.6, 0.4},
                        { 0.2, 0.6, 0.2},
                        { 0.2, 0.4, 0.6},
                        { 0.2, 0.4, 0.4},
                        { 0.2, 0.4, 0.2},
                        { 0.2, 0.2, 0.6},
                        { 0.2, 0.2, 0.4},
                        { 0.2, 0.2, 0.2}
                      };

void GUIServerPlugin::entityToMsg(const ed::EntityConstPtr& e, ed_gui_server::EntityInfo& msg)
{
    ed::ErrorContext errc("entityToMsg");

    msg.id = e->id().str();
    msg.type = e->type();
    msg.existence_probability = e->existenceProbability();
    msg.mesh_revision = e->shapeRevision();

    if (e->has_pose())
    {
        geo::convert(e->pose(), msg.pose);
        msg.has_pose = true;
    }
    else
    {
        msg.has_pose = false;
    }

    if (!e->shape())
    {
        const ed::ConvexHull& ch = e->convexHull();

        if (!ch.points.empty() && e->has_pose())
        {
            const geo::Pose3D& pose = e->pose();

            geo::convert(pose, msg.pose);
            msg.has_pose = true;

            msg.polygon.z_min = ch.z_min;
            msg.polygon.z_max = ch.z_max;

            unsigned int size = ch.points.size();
            msg.polygon.xs.resize(size);
            msg.polygon.ys.resize(size);

            for(unsigned int i = 0; i < size; ++i)
            {
                msg.polygon.xs[i] = ch.points[i].x;
                msg.polygon.ys[i] = ch.points[i].y;
            }
        }
    }

    tue::config::Reader config(e->data());

    if (config.readGroup("color"))
    {
        double r, g, b;
        if (config.value("red", r) && config.value("green", g) && config.value("blue", b))
        {
            msg.color.r = 255 * r;
            msg.color.g = 255 * g;
            msg.color.b = 255 * b;
        }
        
        double a;
        if (config.value("alpha", a))
        {
            msg.color.a = 255 * a;
        }
        
        else
	{
	    msg.color.a = 255;
	}
        
        config.endGroup();
    }


//std::cout << "Gui-server: check flags" << std::endl;
    // HACK! Way of coding that this is a human
    if (e->type() == "human" || e->hasFlag("possible_human"))
    {
        msg.color.a = 1;
        msg.color.r = 2;
        msg.color.g = 3;

        if (e->type() == "human")
            msg.color.b = 4;  // human
        else
            msg.color.b = 5;  // possible human (based on laser)
    }
}

void publishFeatures ( tracking::FeatureProperties& featureProp, unsigned int* ID, visualization_msgs::MarkerArray& markers, ed::UUID entityID, float dt, bool predictEntities, ed_gui_server::objsPosVel& objsInfo, bool Mobidik) // TODO move to ed_rviz_plugins?
{
    visualization_msgs::Marker marker;
    std_msgs::ColorRGBA color;

//     int i_color = *ID % 27;
    color.r = COLORS[0][0];
    color.g = COLORS[0][1];
    color.b = COLORS[0][2];
    color.a = ( float ) 0.5;
    
    std_msgs::ColorRGBA mobidikColor;
    mobidikColor.r = 0;
    mobidikColor.g = 0;
    mobidikColor.b = 1;
    mobidikColor.a = ( float ) 0.5;
    
    if ( featureProp.getFeatureProbabilities().get_pCircle() > featureProp.getFeatureProbabilities().get_pRectangle() ) 
    {
        tracking::Circle circle = featureProp.getCircle();
        if( predictEntities )
        {
                circle.predictAndUpdatePos(dt);
        }

        circle.setMarker ( marker , (*ID)++, color );
        markers.markers.push_back( marker );
        
        float vel2 = pow(circle.get_xVel(), 2.0) + pow(circle.get_yVel(), 2.0);
        if( std::sqrt( vel2 ) > 0.01 )
        {
                circle.setTranslationalVelocityMarker ( marker , (*ID)++);
                markers.markers.push_back( marker );
        }
    } 
    else
    {     
        tracking::Rectangle rectangle = featureProp.getRectangle();
        
        if( predictEntities )
        {
                rectangle.predictAndUpdatePos(dt);
        }
        
        
        if(Mobidik)
        {
                rectangle.setMarker ( marker , (*ID)++, mobidikColor ); 
        }
        else
        {
                rectangle.setMarker ( marker , (*ID)++, color );
        }
        
        markers.markers.push_back( marker );
        
        float vel2 = pow(rectangle.get_xVel(), 2.0) + pow(rectangle.get_yVel(), 2.0);
        if( std::sqrt( vel2 ) > 0.01 )
        {
                rectangle.setTranslationalVelocityMarker ( marker , (*ID)++ );
                markers.markers.push_back( marker );
        }

        if( fabs( rectangle.get_yawVel() ) > 0.01 )
        {
                rectangle.setRotationalVelocityMarker ( marker, (*ID)++ );
                markers.markers.push_back( marker );
        }
    }
    
     if(marker.pose.position.x != marker.pose.position.x || marker.pose.position.y != marker.pose.position.y || marker.pose.position.z != marker.pose.position.z ||
             marker.pose.orientation.x !=  marker.pose.orientation.x || marker.pose.orientation.y !=  marker.pose.orientation.y || marker.pose.orientation.z !=  marker.pose.orientation.z ||
             marker.pose.orientation.w !=  marker.pose.orientation.w || marker.scale.x != marker.scale.x || marker.scale.y != marker.scale.y || marker.scale.z != marker.scale.z )
     {
                ROS_FATAL( "Publishing of object with nan" ); 
                std::cout << "Id = " << entityID  << "Properties are " << std::endl;
                featureProp.printProperties();
                
                exit (EXIT_FAILURE);
     }
        
      // pub all entities on rostopic  
      
      // First, add all rectangular properties
      tracking::Rectangle rectangle = featureProp.getRectangle();
      ed_gui_server::objPosVel objectInfo;
      
      objectInfo.id = entityID.str();
      objectInfo.rectangle.probability = featureProp.getFeatureProbabilities().get_pRectangle();
      
      objectInfo.rectangle.pose.position.x = rectangle.get_x();
      objectInfo.rectangle.pose.position.y = rectangle.get_y();
      objectInfo.rectangle.pose.position.z = rectangle.get_z();
      
      objectInfo.rectangle.xPosStdDev = rectangle.get_P_PosVel()(0,0); 
      objectInfo.rectangle.yPosStdDev = rectangle.get_P_PosVel()(1,1);
      objectInfo.rectangle.yawStdDev = rectangle.get_P_PosVel()(2,2);
      
      objectInfo.rectangle.vel.x = rectangle.get_xVel();
      objectInfo.rectangle.vel.y = rectangle.get_yVel();
      objectInfo.rectangle.vel.z = 0.0;
      objectInfo.rectangle.depth = rectangle.get_d();
      objectInfo.rectangle.width = rectangle.get_w();
      
      objectInfo.rectangle.widthStdDev = rectangle.get_Pdim()(0,0);
      objectInfo.rectangle.depthStdDev = rectangle.get_Pdim()(1,1);
      
      tf2::Quaternion q_rot;
      q_rot.setRPY(rectangle.get_roll(), rectangle.get_pitch(), rectangle.get_yaw());
      tf2::convert(q_rot, objectInfo.rectangle.pose.orientation);
      
      objectInfo.rectangle.yawVel = rectangle.get_yawVel();
      
      // Now, add all circular properties
      tracking::Circle circle = featureProp.getCircle();

      objectInfo.circle.probability = featureProp.getFeatureProbabilities().get_pCircle();
      
      objectInfo.circle.pose.position.x = circle.get_x();
      objectInfo.circle.pose.position.y = circle.get_y();
      objectInfo.circle.pose.position.z = circle.get_z();
      
      objectInfo.circle.xPosStdDev = circle.get_P_PosVel()(0,0);
      objectInfo.circle.yPosStdDev = circle.get_P_PosVel()(1,1);
        
      objectInfo.circle.vel.x = circle.get_xVel();
      objectInfo.circle.vel.y = circle.get_yVel();
      objectInfo.circle.vel.z = 0.0;
      
      objectInfo.circle.radius = circle.get_radius();
      objectInfo.circle.radiusStdDev = circle.get_Pdim()(0,0);

      objsInfo.objects.push_back( objectInfo );        
}

// ----------------------------------------------------------------------------------------------------

GUIServerPlugin::GUIServerPlugin()
{
}

// ----------------------------------------------------------------------------------------------------

GUIServerPlugin::~GUIServerPlugin()
{
}

// ----------------------------------------------------------------------------------------------------

void GUIServerPlugin::initialize(ed::InitData& init)
{
    ed::ErrorContext errc("initialize");

    tue::Configuration& config = init.config;

    std::string robot_name;
    if (config.value("robot_name", robot_name, tue::OPTIONAL))
    {
        robot_.initialize(robot_name);
    }
    
    int i_predict_entities = 0;
    if( config.value("predict_entities", i_predict_entities, tue::OPTIONAL) )
    {
            predict_entities_ = (i_predict_entities != 0);
    }

    ros::NodeHandle nh;

    ros::AdvertiseServiceOptions opt_srv_entities =
            ros::AdvertiseServiceOptions::create<ed_gui_server::QueryEntities>(
                "ed/gui/query_entities", boost::bind(&GUIServerPlugin::srvQueryEntities, this, _1, _2),
                ros::VoidPtr(), &cb_queue_);

    srv_query_entities_ = nh.advertiseService(opt_srv_entities);


    ros::AdvertiseServiceOptions opt_srv_meshes =
            ros::AdvertiseServiceOptions::create<ed_gui_server::QueryMeshes>(
                "ed/gui/query_meshes", boost::bind(&GUIServerPlugin::srvQueryMeshes, this, _1, _2),
                ros::VoidPtr(), &cb_queue_);

    srv_query_meshes_ = nh.advertiseService(opt_srv_meshes);

    ros::AdvertiseServiceOptions opt_srv_get_entity_info =
            ros::AdvertiseServiceOptions::create<ed_gui_server::GetEntityInfo>(
                "ed/gui/get_entity_info", boost::bind(&GUIServerPlugin::srvGetEntityInfo, this, _1, _2),
                ros::VoidPtr(), &cb_queue_);

    srv_get_entity_info_ = nh.advertiseService(opt_srv_get_entity_info);

    ros::AdvertiseServiceOptions opt_srv_interact =
            ros::AdvertiseServiceOptions::create<ed_gui_server::Interact>(
                "ed/gui/interact", boost::bind(&GUIServerPlugin::srvInteract, this, _1, _2),
                ros::VoidPtr(), &cb_queue_);

    srv_interact_ = nh.advertiseService(opt_srv_interact);

    pub_entities_ = nh.advertise<ed_gui_server::EntityInfos>("ed/gui/entities", 1);
    
    ObjectMarkers_pub_ = nh.advertise<visualization_msgs::MarkerArray> ( "ed/gui/objectMarkers", 3 );  // TODO transform information to message via ed/gui/entities and puplish to rviz via rviz_publisher.cpp
    // TODO standard initialization of all custom properties at all plugins?!
    
   ObjectPosVel_pub_ = nh.advertise<ed_gui_server::objsPosVel> ( "ed/gui/objectPosVel", 3 );  // TODO transform information to message via ed/gui/entities and puplish to rviz via rviz_publisher.cpp
    // TODO standard initialization of all custom properties at all plugins?!
    
    init.properties.registerProperty ( "Feature", featureProperties_, new FeaturPropertiesInfo );
}

// ----------------------------------------------------------------------------------------------------

void GUIServerPlugin::process(const ed::WorldModel& world, ed::UpdateRequest& req)
{
//         std::cout << "Gui server: process start." << std::endl;
    ed::ErrorContext errc("process");

    world_model_ = &world;
    cb_queue_.callAvailable();

    ed_gui_server::EntityInfos entities_msg;

    entities_msg.header.stamp = ros::Time::now();
    entities_msg.header.frame_id = "/map";

    entities_msg.entities.resize(world_model_->numEntities());

    unsigned int i = 0;
    unsigned int marker_ID = 0;
    visualization_msgs::MarkerArray markerArray;
    ed_gui_server::objsPosVel objsArray;
    
    // delete old stuf before publishing updated markers. This prevents that old stuff is visualized.
    visualization_msgs::MarkerArray deleteAllMarkerArray;
    visualization_msgs::Marker deleteAllMarker;
    deleteAllMarker.action = visualization_msgs::Marker::DELETEALL;
    deleteAllMarkerArray.markers.push_back( deleteAllMarker );
    ObjectMarkers_pub_.publish( deleteAllMarkerArray );

    
    for(ed::WorldModel::const_iterator it = world_model_->begin(); it != world_model_->end(); ++it)
    {
        const ed::EntityConstPtr& e = *it;

        if ( !e->hasFlag ( "self" ) ) 
        {
            entityToMsg ( e, entities_msg.entities[i++] );
        }
     
        std::string laserID = "-laserTracking";
        if ( ! ( e->id().str().length() < laserID.length() ) ) 
        {
            if ( e->id().str().substr ( e->id().str().length() - laserID.length() ) == laserID ) // entity described by laser
            {
                if(e->property ( featureProperties_ ))
                {
                    tracking::FeatureProperties measuredProperty = e->property ( featureProperties_ );
                    float dt = ros::Time::now().toSec() - e->lastUpdateTimestamp();
                    publishFeatures ( measuredProperty, &marker_ID, markerArray, e->id(), dt, predict_entities_, objsArray, e->hasFlag("Mobidik") );
                }  else {
                        std::cout << "GUI-server warn" << std::endl;
                }            
            }
        }
    }

    robot_.getEntities(entities_msg.entities);
    pub_entities_.publish(entities_msg);
    ObjectMarkers_pub_.publish( markerArray );
    
    objsArray.header.stamp = ros::Time::now();
    objsArray.header.frame_id = "/map";
    
    ObjectPosVel_pub_.publish ( objsArray );
}

// ----------------------------------------------------------------------------------------------------

bool GUIServerPlugin::srvQueryEntities(const ed_gui_server::QueryEntities::Request& ros_req, ed_gui_server::QueryEntities::Response& ros_res)
{
    for(ed::WorldModel::const_iterator it = world_model_->begin(); it != world_model_->end(); ++it)
    {       
        const ed::EntityConstPtr& e = *it;

        const geo::Pose3D* pose = 0;
        if (e->has_pose())
            pose = &e->pose();

        if (!pose)
            continue;

        float pos_x = pose->t.x;
        float pos_y = pose->t.y;

        if (ros_req.area_min.x < pos_x && pos_x < ros_req.area_max.x
                && ros_req.area_min.y < pos_y && pos_y < ros_req.area_max.y)
        {
            ros_res.entities.push_back(ed_gui_server::EntityInfo());
            ed_gui_server::EntityInfo& info = ros_res.entities.back();

            info.id = e->id().str();
            info.mesh_revision = e->shapeRevision();
            geo::convert(*pose, info.pose);
        }
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------

enum ImageCompressionType
{
    IMAGE_COMPRESSION_JPG,
    IMAGE_COMPRESSION_PNG
};

bool imageToBinary(const cv::Mat& image, std::vector<unsigned char>& data, ImageCompressionType compression_type)
{
    if (compression_type == IMAGE_COMPRESSION_JPG)
    {
        // OpenCV compression settings
        std::vector<int> rgb_params;
        rgb_params.resize(3, 0);

        rgb_params[0] = CV_IMWRITE_JPEG_QUALITY;
        rgb_params[1] = 95; // default is 95

        // Compress image
        if (!cv::imencode(".jpg", image, data, rgb_params)) {
            std::cout << "RGB image compression failed" << std::endl;
            return false;
        }
    }
    else if (compression_type == IMAGE_COMPRESSION_PNG)
    {
        std::vector<int> params;
        params.resize(3, 0);

        params[0] = CV_IMWRITE_PNG_COMPRESSION;
        params[1] = 1;

        if (!cv::imencode(".png", image, data, params)) {
            std::cout << "PNG image compression failed" << std::endl;
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------

bool GUIServerPlugin::srvGetEntityInfo(const ed_gui_server::GetEntityInfo::Request& ros_req,
                                       ed_gui_server::GetEntityInfo::Response& ros_res)
{
    ed::EntityConstPtr e = world_model_->getEntity(ros_req.id);
    if (!e)
        return true;

    ros_res.type = e->type();

    // TODO: get affordances from entity
    ros_res.affordances.push_back("navigate-to");
    ros_res.affordances.push_back("pick-up");
    ros_res.affordances.push_back("place");

    const geo::Pose3D* pose = 0;
    if (e->has_pose())
        pose = &e->pose();

    if (pose)
    {
        std::stringstream ss_pose;
        ss_pose << "(" << pose->t.x << ", " << pose->t.y << ", " << pose->t.z << ")";
        ros_res.property_names.push_back("position");
        ros_res.property_values.push_back(ss_pose.str());
    }

    ed::MeasurementConstPtr m = e->lastMeasurement();
    if (m)
    {
        const cv::Mat& rgb_image = m->image()->getRGBImage();
        const ed::ImageMask& image_mask = m->imageMask();

        cv::Mat rgb_image_masked(rgb_image.rows, rgb_image.cols, CV_8UC3, cv::Scalar(0, 0, 0));

        cv::Point min(rgb_image.cols, rgb_image.rows);
        cv::Point max(0, 0);
        for(ed::ImageMask::const_iterator it = image_mask.begin(rgb_image.cols); it != image_mask.end(); ++it)
        {
            const cv::Point2i& p =*it;
            rgb_image_masked.at<cv::Vec3b>(p) = rgb_image.at<cv::Vec3b>(p);

            min.x = std::min(min.x, p.x);
            min.y = std::min(min.y, p.y);
            max.x = std::max(max.x, p.x);
            max.y = std::max(max.y, p.y);
        }

        imageToBinary(rgb_image_masked, ros_res.measurement_image, IMAGE_COMPRESSION_JPG);

        // Add border to roi
        min.x = std::max(0, min.x - ros_req.measurement_image_border);
        min.y = std::max(0, min.y - ros_req.measurement_image_border);
        max.x = std::min(rgb_image.cols - 1, max.x + ros_req.measurement_image_border);
        max.y = std::min(rgb_image.rows - 1, max.y + ros_req.measurement_image_border);

        imageToBinary(rgb_image(cv::Rect(min, max)), ros_res.measurement_image_unmasked, IMAGE_COMPRESSION_JPG);
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------

bool GUIServerPlugin::srvQueryMeshes(const ed_gui_server::QueryMeshes::Request& ros_req,
                                      ed_gui_server::QueryMeshes::Response& ros_res)
{
    for(unsigned int i = 0; i < ros_req.entity_ids.size(); ++i)
    {
        const std::string& id = ros_req.entity_ids[i];

        geo::ShapeConstPtr shape = robot_.getShape(id);
        int shape_revision = 1;

        ed::EntityConstPtr e;
        if (!shape)
        {
            e = world_model_->getEntity(id);
            if (e)
            {
                shape = e->shape();
                shape_revision = e->shapeRevision();
            }
            else
                ros_res.error_msg += "Unknown entity: '" + id + "'.\n";
        }

        if (shape)
        {
            ros_res.entity_geometries.push_back(ed_gui_server::EntityMeshAndAreas());
            ed_gui_server::EntityMeshAndAreas& entity_geometry = ros_res.entity_geometries.back();

            entity_geometry.id = id;

            // Mesh revision
            entity_geometry.mesh.revision = shape_revision;

            const std::vector<geo::Vector3>& vertices = shape->getMesh().getPoints();

            // Triangles
            const std::vector<geo::TriangleI>& triangles = shape->getMesh().getTriangleIs();
            entity_geometry.mesh.vertices.resize(triangles.size() * 9);
            for(unsigned int i = 0; i < triangles.size(); ++i)
            {
                const geo::TriangleI& t = triangles[i];
                const geo::Vector3& v1 = vertices[t.i1_];
                const geo::Vector3& v2 = vertices[t.i2_];
                const geo::Vector3& v3 = vertices[t.i3_];

                unsigned int i9 = i * 9;

                entity_geometry.mesh.vertices[i9] = v1.x;
                entity_geometry.mesh.vertices[i9 + 1] = v1.y;
                entity_geometry.mesh.vertices[i9 + 2] = v1.z;
                entity_geometry.mesh.vertices[i9 + 3] = v2.x;
                entity_geometry.mesh.vertices[i9 + 4] = v2.y;
                entity_geometry.mesh.vertices[i9 + 5] = v2.z;
                entity_geometry.mesh.vertices[i9 + 6] = v3.x;
                entity_geometry.mesh.vertices[i9 + 7] = v3.y;
                entity_geometry.mesh.vertices[i9 + 8] = v3.z;
            }

            // Render areas if e
            if (e)
            {
                geo::Shape area_shape;
                tue::config::Reader r(e->data());

                if (r.readArray("areas"))
                {
                    while(r.nextArrayItem())
                    {
                        std::string a_name;
                        if (!r.value("name", a_name))
                            continue;

                        if (ed::deserialize(r, "shape", area_shape))
                        {
                            entity_geometry.areas.push_back(ed_gui_server::Area());
                            ed_gui_server::Area& entity_area = entity_geometry.areas.back();

                            entity_area.name = a_name;

                            const std::vector<geo::Vector3>& vertices = area_shape.getMesh().getPoints();

                            // Triangles

                            const std::vector<geo::TriangleI>& triangles = area_shape.getMesh().getTriangleIs();
                            entity_area.mesh.vertices.resize(triangles.size() * 9);
                            for(unsigned int i = 0; i < triangles.size(); ++i)
                            {
                                const geo::TriangleI& t = triangles[i];
                                const geo::Vector3& v1 = vertices[t.i1_];
                                const geo::Vector3& v2 = vertices[t.i2_];
                                const geo::Vector3& v3 = vertices[t.i3_];

                                unsigned int i9 = i * 9;

                                entity_area.mesh.vertices[i9] = v1.x;
                                entity_area.mesh.vertices[i9 + 1] = v1.y;
                                entity_area.mesh.vertices[i9 + 2] = v1.z;
                                entity_area.mesh.vertices[i9 + 3] = v2.x;
                                entity_area.mesh.vertices[i9 + 4] = v2.y;
                                entity_area.mesh.vertices[i9 + 5] = v2.z;
                                entity_area.mesh.vertices[i9 + 6] = v3.x;
                                entity_area.mesh.vertices[i9 + 7] = v3.y;
                                entity_area.mesh.vertices[i9 + 8] = v3.z;
                            }
                        }
                    }
                    r.endArray();
                }
            }
        }        
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------

void GUIServerPlugin::storeMeasurement(const std::string& id, const std::string& type)
{
    ed::EntityConstPtr e = world_model_->getEntity(id);
    if (e)
    {
        char const* home = getenv("HOME");
        if (home)
        {
            boost::filesystem::path dir(std::string(home) + "/.ed/measurements/" + type);
            boost::filesystem::create_directories(dir);

            std::string filename = dir.string() + "/" + ed::Entity::generateID().str();
            ed::write(filename, *e);

            std::cout << "Writing entity info to '" << filename << "'." << std::endl;
        }
    }
    else
        std::cout << "Entity with id " << id << " does not exist!" << std::endl;
}

// ----------------------------------------------------------------------------------------------------

bool GUIServerPlugin::srvInteract(const ed_gui_server::Interact::Request& ros_req,
                                ed_gui_server::Interact::Response& ros_res)
{
    std::cout << "[GUIServerPlugin] Received command: " << ros_req.command_yaml << std::endl;

    tue::Configuration params;
    tue::config::loadFromYAMLString(ros_req.command_yaml, params);

    std::string action;
    if (params.value("action", action))
    {
        if (action == "store")
        {
            std::string id, type;
            if (params.value("id", id) && params.value("type", type))
                storeMeasurement(id, type);
            else
                std::cout << "Please specify an id and a type!" << std::endl;
        }
    }

    if (params.hasError())
        ros_res.result_json = "{ error: \"" + params.error() + "\" }";
    else
        ros_res.result_json = "{}";
}

ED_REGISTER_PLUGIN(GUIServerPlugin)
