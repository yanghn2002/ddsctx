#include "demo.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ddsctx.hpp"

int reader_have_data = 0;
char data_buffer[256];

void topic_callback(int event, const dds_domainid_t domainid, const char* topic) {
    switch(event) {
        default: printf("TOPIC: \\x%x\n", event);
    }
}
void reader_callback(int event, const dds_domainid_t domainid, const char* topic) {
    switch(event) {
        case DDSCTX_READER_ON_DATA_AVAILABLE:
            reader_have_data = 1;
            break;
        case DDSCTX_READER_ON_SUBSCRIPTION_MATCHED:
            printf("SUBSCRIPTION_MATCHED: domain=%d, topic=%s\n", domainid, topic);
            break;
        default: printf("READER: \\x%x\n", event);
    }
}
void writer_callback(int event, const dds_domainid_t domainid, const char* topic) {
    switch(event) {
        case DDSCTX_WRITER_ON_PUBLICATION_MATCHED:
            printf("PUBLICATION_MATCHED: domain=%d, topic=%s\n", domainid, topic);
            break;
        default: printf("WRITER: \\x%x\n", event);
    }
}

int main_sub(int argc, char* argv[]) {

    enum samples { demomsg_0 };
    ddsctx_sample(demomsg_0, sizeof(DemoMsg), &DemoMsg_desc);
    ddsctx_reader(DDS_DOMAIN_DEFAULT, "topic_demo", "qos_demo");
    ddsctx_set_reader_callback(DDS_DOMAIN_DEFAULT, "topic_demo", reader_callback);
    
    while(1) {
        if(reader_have_data) {
            ddsctx_take(DDS_DOMAIN_DEFAULT, "topic_demo", demomsg_0);
            DemoMsg* msg = (DemoMsg*)ddsctx_get_data(demomsg_0);
            if(ddsctx_get_valid(demomsg_0)) printf("SUB: data=%s\n", msg->data);
            reader_have_data = 0;
        }
    }

    return 0;

}

int main_pub(int argc, char* argv[]) {
    
    DemoMsg msg;

    ddsctx_writer(DDS_DOMAIN_DEFAULT, "topic_demo", "qos_demo");
    ddsctx_set_writer_callback(DDS_DOMAIN_DEFAULT, "topic_demo", writer_callback);

    sleep(1);
    unsigned data = 0;
    msg.data = data_buffer;
    while(1) {
        snprintf(data_buffer, sizeof(data_buffer), "DDS_MESSAGE_%u", data++);
        printf("PUB: data=%s\n", msg.data);
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
