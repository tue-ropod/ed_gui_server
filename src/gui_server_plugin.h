#ifndef ED_GUI_SERVER_PLUGIN_H_
#define ED_GUI_SERVER_PLUGIN_H_

#include <ed/plugin.h>

#include <ed_gui_server/QueryEntities.h>
#include <ed_gui_server/QueryMeshes.h>
#include <ed_gui_server/Interact.h>
#include <ed_gui_server/GetEntityInfo.h>

#include <ros/callback_queue.h>
#include <ros/service_server.h>
#include <ros/publisher.h>

#include "robot.h"

#include "ed/convex_hull.h"
#include "ed/convex_hull_calc.h"

#include "ed/featureProperties_info.h"

class GUIServerPlugin : public ed::Plugin
{

public:

    GUIServerPlugin();

    virtual ~GUIServerPlugin();

    void initialize(ed::InitData& init);

    void process(const ed::WorldModel& world, ed::UpdateRequest& req);

private:

    //

    const ed::WorldModel* world_model_;

    ros::CallbackQueue cb_queue_;

    ros::Publisher pub_entities_;

    ros::ServiceServer srv_query_meshes_;

    gui::Robot robot_;

    bool srvQueryMeshes(const ed_gui_server::QueryMeshes::Request& ros_req,
                         ed_gui_server::QueryMeshes::Response& ros_res);

    ros::ServiceServer srv_query_entities_;

    bool srvQueryEntities(const ed_gui_server::QueryEntities::Request& ros_req,
                          ed_gui_server::QueryEntities::Response& ros_res);

    ros::ServiceServer srv_get_entity_info_;

    bool srvGetEntityInfo(const ed_gui_server::GetEntityInfo::Request& ros_req,
                          ed_gui_server::GetEntityInfo::Response& ros_res);

    ros::ServiceServer srv_interact_;

    bool srvInteract(const ed_gui_server::Interact::Request& ros_req,
                          ed_gui_server::Interact::Response& ros_res);

    void storeMeasurement(const std::string& id, const std::string& type);


    void entityToMsg(const ed::EntityConstPtr& e, ed_gui_server::EntityInfo& msg);
    
    unsigned int djb2(const std::string& str);
    
    ros::Publisher ObjectMarkers_pub_;
    
    ed::PropertyKey<ed::tracking::FeatureProperties> featureProperties_;
    
    
};

#endif
