#include "demo.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ddsctx.hpp"

int reader_have_data = 0;

void reader_callback(
    int event,
    const dds_domainid_t domainid,
    const char* topic
) {
    if(event == READER_ON_DATA_AVAILABLE) reader_have_data = 1;
}

int main_sub(int argc, char* argv[]) {

    enum samples { demomsg_0 };
    ddsctx_sample(demomsg_0, sizeof(DemoMsg), &DemoMsg_desc);
    ddsctx_reader(DDS_DOMAIN_DEFAULT, "topic_demo", "qos_demo");
    ddsctx_set_reader_callback(DDS_DOMAIN_DEFAULT, "topic_demo", reader_callback);
    
    while(1) {
        if(reader_have_data) {
            ddsctx_read(DDS_DOMAIN_DEFAULT, "topic_demo", demomsg_0);
            DemoMsg* msg = (DemoMsg*)ddsctx_get_data(demomsg_0);
            if(ddsctx_get_valid(demomsg_0))
                printf ("data=%d\n", msg->data);
            reader_have_data = 0;
        }
    }

    return 0;

}

int main_pub(int argc, char* argv[]) {
    
    DemoMsg msg;

    ddsctx_writer(DDS_DOMAIN_DEFAULT, "topic_demo", "qos_demo");

    sleep(1);
    msg.data = 0;
    while(1) {
        msg.data++;
        printf("data=%d\n", msg.data);
        ddsctx_send(DDS_DOMAIN_DEFAULT, "topic_demo", &msg);
        sleep(1);
    }

    return 0;
}

int main(int argc, char* argv[]) {

    dds_qset_reliability(ddsctx_qos("qos_demo"), DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
    ddsctx_topic(DDS_DOMAIN_DEFAULT, &DemoMsg_desc, "topic_demo", "qos_demo");

    if(argc == 1) {
        if(!strcmp(argv[0], "pub"))
            return main_pub(argc, argv);
        if(!strcmp(argv[0], "sub"))
            return main_sub(argc, argv);
    }

    printf("Usage: %s [pub|sub]\n", argv[0]);
    return 1;

}