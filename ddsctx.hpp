#ifndef __DDSCTX_HPP
#define __DDSCTX_HPP

#ifdef DDSCTX_OBJECT
    #ifdef __cplusplus
        #define __DDSCTX_OBJECT
    #else
        #error DDSCTX_OBJECT defined without __cplusplus
    #endif
#endif

#include <dds/dds.h>

enum ddsctx_event {
    DDSCTX_TOPIC_ON_INCONSISTENT_TOPIC          = 0x00,
    DDSCTX_READER_ON_DATA_AVAILABLE             = 0x10,
    DDSCTX_READER_ON_SUBSCRIPTION_MATCHED       = 0x11,
    DDSCTX_READER_ON_SAMPLE_LOST                = 0x12,
    DDSCTX_READER_ON_SAMPLE_REJECTED            = 0x13,
    DDSCTX_READER_ON_LIVELINESS_CHANGED         = 0x14,
    DDSCTX_READER_ON_REQUESTED_DEADLINE_MISSED  = 0x15,
    DDSCTX_READER_ON_REQUESTED_INCOMPATIBLE_QOS = 0x16,
    DDSCTX_WRITER_ON_PUBLICATION_MATCHED        = 0x20,
    DDSCTX_WRITER_ON_LIVELINESS_LOST            = 0x21,
    DDSCTX_WRITER_ON_OFFERED_DEADLINE_MISSED    = 0x22,
    DDSCTX_WRITER_ON_OFFERED_INCOMPATIBLE_QOS   = 0x23,
};

typedef void(ddsctx_callback_t)(int, const dds_domainid_t, const char*, const void*);

#ifndef __DDSCTX_OBJECT
#ifdef __cplusplus
extern "C" {
#endif

extern void ddsctx_sample(
    const int,
    const size_t,
    const dds_topic_descriptor_t*
);
extern dds_qos_t* ddsctx_qos(const char*);
extern dds_entity_t ddsctx_domain(const dds_domainid_t);
extern dds_entity_t ddsctx_topic(
    const dds_domainid_t,
    const dds_topic_descriptor_t*,
    const char*,
    const char*
);
extern dds_entity_t ddsctx_reader(
    const dds_domainid_t,
    const char*,
    const char*
);
extern dds_entity_t ddsctx_writer(
    const dds_domainid_t,
    const char*,
    const char*
);
extern void ddsctx_send(
    const dds_domainid_t,
    const char*,
    void*
);
extern void ddsctx_read(
    const dds_domainid_t,
    const char*,
    const int
);
extern void ddsctx_take(
    const dds_domainid_t,
    const char*,
    const int
);
extern void ddsctx_set_topic_callback(
    const dds_domainid_t,
    const char*,
    ddsctx_callback_t callback
);
extern void ddsctx_set_reader_callback(
    const dds_domainid_t,
    const char*,
    ddsctx_callback_t callback
);
extern void ddsctx_set_writer_callback(
    const dds_domainid_t,
    const char*,
    ddsctx_callback_t callback
);
extern void* ddsctx_get_data(const int);
extern int ddsctx_get_valid(const int);

#ifdef __cplusplus
}
#endif
#else//__DDSCTX_OBJECT

#include <map>
#include <string>
#include <utility>
#include <stdexcept>

class DDSError: public std::runtime_error {
    
    dds_return_t _error;
    
    public:
        DDSError(const char* func, dds_return_t ret)
        : std::runtime_error(func), _error(ret) {}

        const char* what() const noexcept override {
            static std::string msg = std::runtime_error::what();
            msg += " ("+std::to_string(_error) + "): "+dds_strretcode(-_error);
            return msg.c_str();
        }

};

#define DDSCTX_INSTANCE(X) DDS& X = DDS::instance()

class DDS final {

    class Sample final {

        void* _sample[1];
        dds_sample_info_t _info[1];
        const dds_topic_descriptor_t* _descriptor;
        bool _alloced;

        void _alloced_check(void) {
            if(!_alloced) throw std::logic_error("invaild sample");
        }

        public:

            Sample(void): _alloced(false) {
                _sample[0] = nullptr;
            }

            void operator()(const size_t size, const dds_topic_descriptor_t* descriptor) {
                _descriptor = descriptor;
                _sample[0] = dds_alloc(size);
                _alloced = true;
            }

            dds_sample_info_t* info(void) {
                _alloced_check();
                return _info;
            }

            void** sample(void) {
                _alloced_check();
                return _sample;
            }

            ~Sample(void) {
                if(_sample[0])
                dds_sample_free(_sample[0], _descriptor, DDS_FREE_ALL);
            }

    };

    std::map<int, Sample> _sample;
    std::map<std::string, dds_qos_t*> _qos;
    std::map<dds_domainid_t, dds_entity_t> _domain;
    std::map<
        std::pair<dds_domainid_t, std::string>,
        std::tuple<dds_entity_t, dds_listener_t*, ddsctx_callback_t*>
    > _topic;
    std::map<
        std::pair<dds_domainid_t, std::string>,
        std::tuple<dds_entity_t, dds_listener_t*, ddsctx_callback_t*>
    > _reader;
    std::map<
        std::pair<dds_domainid_t, std::string>,
        std::tuple<dds_entity_t, dds_listener_t*, ddsctx_callback_t*>
    > _writer;
    std::map<dds_entity_t, std::pair<dds_domainid_t, std::string>> _entities;     
    
    DDS(void) = default;
    ~DDS(void) {
        for(auto& [domainid, participant]: _domain) dds_delete(participant);
        for(auto& [name, qos]: _qos) dds_delete_qos(qos);
        for(auto& [demainid_topic, _]: _reader) dds_delete_listener(std::get<1>(_));
        for(auto& [demainid_topic, _]: _writer) dds_delete_listener(std::get<1>(_));
    }

    std::logic_error _unknow_sample(const int index) {
        return
            std::logic_error(
                "unknow sample: \""+std::to_string(index)+"\"");
    }
    std::logic_error _unknow_topic(const std::string& topic, const dds_domainid_t domainid) {
        return
            std::logic_error(
                "unknow topic: \""+topic+"\" in domain "+std::to_string(domainid));
    }
    std::logic_error _unknow_reader(const std::string& topic, const dds_domainid_t domainid) {
        return
            std::logic_error(
                "unknow reader for topic: \""+topic+"\" in domain "+std::to_string(domainid));
    }
    std::logic_error _unknow_writer(const std::string& topic, const dds_domainid_t domainid) {
        return
            std::logic_error(
                "unknow writer for topic: \""+topic+"\" in domain "+std::to_string(domainid));
    }

    public:

        static DDS& instance(void) {
            static DDS _dds;
            return _dds;
        }
        DDS(const DDS&) = delete;
        DDS& operator=(const DDS&) = delete;
        DDS(DDS&&) = delete;
        DDS& operator=(DDS&&) = delete;

        static void sample(
            const int index,
            const size_t size,
            const dds_topic_descriptor_t* descriptor
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._sample.count(index)) dds._sample[index](size, descriptor);

        }
        
        static dds_qos_t* qos(const std::string& name) {
            
            DDSCTX_INSTANCE(dds);

            if(!dds._qos.count(name)) dds._qos[name] = dds_create_qos();
            return dds._qos[name];

        }

        static dds_entity_t domain(const dds_domainid_t domainid) {

            DDSCTX_INSTANCE(dds);

            if(!dds._domain.count(domainid)) {
                dds_entity_t participant = dds_create_participant(domainid, NULL, NULL);
                if(participant < 0) throw DDSError("dds_create_participant", participant);
                dds._domain[domainid] = participant;
            }
            return dds._domain[domainid];

        }

        static dds_entity_t topic(
            const dds_domainid_t domainid,
            const dds_topic_descriptor_t* descriptor,
            const std::string& name,
            const std::string& qos
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._topic.count({domainid, name})) {
                dds_listener_t* listener = dds_create_listener(NULL);
                dds_lset_inconsistent_topic(listener, _on_inconsistent_topic);
                dds_entity_t topic = dds_create_topic(
                    dds.domain(domainid),
                    descriptor,
                    name.c_str(),
                    dds.qos(qos),
                    listener
                );
                if(topic < 0) throw DDSError("dds_create_topic", topic);
                dds._topic[{domainid, name}] = {topic, listener, nullptr};
                dds._entities[topic] = {domainid, name};
                return topic;
            } else return std::get<0>(dds._topic[{domainid, name}]);

        }

        static dds_entity_t reader(
            const dds_domainid_t domainid,
            const std::string& topic,
            const std::string& qos
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._reader.count({domainid, topic})) {
                if(!dds._topic.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
                dds_listener_t* listener = dds_create_listener(NULL);
                dds_lset_data_available(listener, _on_data_available);
                dds_lset_subscription_matched(listener, _on_subscription_matched);
                dds_lset_sample_lost(listener, _on_sample_lost);
                dds_lset_sample_rejected(listener, _on_sample_rejected);
                dds_lset_liveliness_changed(listener, _on_liveliness_changed);
                dds_lset_requested_deadline_missed(listener, _on_requested_deadline_missed);
                dds_lset_requested_incompatible_qos(listener, _on_requested_incompatible_qos);
                auto& [topic_entity, topic_listener, callback] = dds._topic[{domainid, topic}];
                dds_entity_t reader = dds_create_reader(
                    dds.domain(domainid),
                    topic_entity,
                    dds.qos(qos),
                    listener
                );
                if(reader < 0) throw DDSError("dds_create_reader", reader);
                dds._reader[{domainid, topic}] = {reader, listener, nullptr};
                dds._entities[reader] = {domainid, topic};
                return reader;
            } else return std::get<0>(dds._reader[{domainid, topic}]);

        }

        static dds_entity_t writer(
            const dds_domainid_t domainid,
            const std::string& topic,
            const std::string& qos
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._writer.count({domainid, topic})) {
                if(!dds._topic.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
                dds_listener_t* listener = dds_create_listener(NULL);
                dds_lset_publication_matched(listener, _on_publication_matched);
                dds_lset_liveliness_lost(listener, _on_liveliness_lost);
                dds_lset_offered_deadline_missed(listener, _on_offered_deadline_missed);
                dds_lset_offered_incompatible_qos(listener, _on_offered_incompatible_qos);
                auto& [topic_entity, topic_listener, callback] = dds._topic[{domainid, topic}];
                dds_entity_t writer = dds_create_writer(
                    dds.domain(domainid),
                    topic_entity,
                    dds.qos(qos),
                    listener
                );
                if(writer < 0) throw DDSError("dds_create_writer", writer);
                dds._writer[{domainid, topic}] = {writer, listener, nullptr};
                dds._entities[writer] = {domainid, topic};
                return writer;
            } else return std::get<0>(dds._writer[{domainid, topic}]);

        }
        
        static void send(
            const dds_domainid_t domainid,
            const std::string& topic,
            void* data
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._writer.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
            dds_return_t write = dds_write(
                std::get<0>(dds._writer[{domainid, topic}]),
                data
            );
            if(write < 0) throw DDSError("dds_write", write);

        }
        
        static void read(
            const dds_domainid_t domainid,
            const std::string& topic,
            const int sample
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._reader.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
            if(!dds._sample.count(sample)) throw dds._unknow_sample(sample);
            Sample& sample_obj = dds._sample[sample];
            dds_return_t read = dds_read(
                std::get<0>(dds._reader[{domainid, topic}]),
                sample_obj.sample(),
                sample_obj.info(),
                1, 1
            );
            if(read < 0) throw DDSError("dds_read", read);

        }
        
        static void take(
            const dds_domainid_t domainid,
            const std::string& topic,
            const int sample
        ) {

            DDSCTX_INSTANCE(dds);

            if(!dds._reader.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
            if(!dds._sample.count(sample)) throw dds._unknow_sample(sample);
            Sample& sample_obj = dds._sample[sample];
            dds_return_t take = dds_take(
                std::get<0>(dds._reader[{domainid, topic}]),
                sample_obj.sample(),
                sample_obj.info(),
                1, 1
            );
            if(read < 0) throw DDSError("dds_take", take);

        }

        static void set_topic_callback(
            const dds_domainid_t domainid,
            const std::string& topic,
            void(callback)(int, const dds_domainid_t, const char*, const void*)
        ) {
            
            DDSCTX_INSTANCE(dds);
            
            if(!dds._topic.count({domainid, topic})) throw dds._unknow_topic(topic, domainid);
            std::get<2>(dds._topic[{domainid, topic}]) = callback;

        }

        static void set_reader_callback(
            const dds_domainid_t domainid,
            const std::string& topic,
            void(callback)(int, const dds_domainid_t, const char*, const void*)
        ) {
            
            DDSCTX_INSTANCE(dds);
            
            if(!dds._reader.count({domainid, topic})) throw dds._unknow_reader(topic, domainid);
            std::get<2>(dds._reader[{domainid, topic}]) = callback;

        }

        static void set_writer_callback(
            const dds_domainid_t domainid,
            const std::string& topic,
            void(callback)(int, const dds_domainid_t, const char*, const void*)
        ) {
            
            DDSCTX_INSTANCE(dds);
            
            if(!dds._writer.count({domainid, topic})) throw dds._unknow_writer(topic, domainid);
            std::get<2>(dds._writer[{domainid, topic}]) = callback;

        }

        static void* get_data(int sample) {

            DDSCTX_INSTANCE(dds);
            
            if(!dds._sample.count(sample)) throw dds._unknow_sample(sample);
            return dds._sample[sample].sample()[0];
        }

        static int get_valid(int sample) {
            
            DDSCTX_INSTANCE(dds);
            
            if(!dds._sample.count(sample)) throw dds._unknow_sample(sample);
            return dds._sample[sample].info()[0].valid_data == 1 ? 1 : 0;
        }

        private:

#define __DDSCTX_EVENT_CALLBACK(ENTITY, EVENT, DATA)\
    DDSCTX_INSTANCE(dds);\
    auto& [domainid, topic_name] = dds._entities[ENTITY];\
    auto& callback = std::get<2>(dds._##ENTITY[{domainid, topic_name}]);\
    if(callback) callback(EVENT, domainid, topic_name.c_str(), DATA);
            static void _on_inconsistent_topic
            (dds_entity_t topic, const dds_inconsistent_topic_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(topic, DDSCTX_TOPIC_ON_INCONSISTENT_TOPIC, &status) }
            static void _on_data_available
            (dds_entity_t reader, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_DATA_AVAILABLE, NULL) }
            static void _on_subscription_matched
            (dds_entity_t reader, const dds_subscription_matched_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_SUBSCRIPTION_MATCHED, &status) }
            static void _on_sample_lost
            (dds_entity_t reader, const dds_sample_lost_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_SAMPLE_LOST, &status) }
            static void _on_sample_rejected
            (dds_entity_t reader, const dds_sample_rejected_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_SAMPLE_REJECTED, &status) }
            static void _on_liveliness_changed
            (dds_entity_t reader, const dds_liveliness_changed_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_LIVELINESS_CHANGED, &status) }
            static void _on_requested_deadline_missed
            (dds_entity_t reader, const dds_requested_deadline_missed_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_REQUESTED_DEADLINE_MISSED, &status) }
            static void _on_requested_incompatible_qos
            (dds_entity_t reader, const dds_requested_incompatible_qos_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(reader, DDSCTX_READER_ON_REQUESTED_INCOMPATIBLE_QOS, &status) }
            static void _on_publication_matched
            (dds_entity_t writer, const dds_publication_matched_status_t  status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(writer, DDSCTX_WRITER_ON_PUBLICATION_MATCHED, &status) }
            static void _on_liveliness_lost
            (dds_entity_t writer, const dds_liveliness_lost_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(writer, DDSCTX_WRITER_ON_LIVELINESS_LOST, &status) }
            static void _on_offered_deadline_missed
            (dds_entity_t writer, const dds_offered_deadline_missed_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(writer, DDSCTX_WRITER_ON_OFFERED_DEADLINE_MISSED, &status) }
            static void _on_offered_incompatible_qos
            (dds_entity_t writer, const dds_offered_incompatible_qos_status_t status, void* arg)
            { __DDSCTX_EVENT_CALLBACK(writer, DDSCTX_WRITER_ON_OFFERED_INCOMPATIBLE_QOS, &status) }

};

extern "C" void ddsctx_sample(
    const int index,
    const size_t size,
    const dds_topic_descriptor_t* descriptor
)   { DDS::sample(index, size, descriptor); }
extern "C" dds_qos_t* ddsctx_qos(const char* name)
    { return DDS::qos(name); }
extern "C" dds_entity_t ddsctx_domain(const dds_domainid_t domainid)
    { return DDS::domain(domainid); }
extern "C" dds_entity_t ddsctx_topic(
    const dds_domainid_t domainid,
    const dds_topic_descriptor_t* descriptor,
    const char* name,
    const char* qos
)   { return DDS::topic(domainid, descriptor, name, qos); }
extern "C" dds_entity_t ddsctx_reader(
    const dds_domainid_t domainid,
    const char* topic,
    const char* qos
)   { return DDS::reader(domainid, topic, qos); }
extern "C" dds_entity_t ddsctx_writer(
    const dds_domainid_t domainid,
    const char* topic,
    const char* qos
)   { return DDS::writer(domainid, topic, qos); }
extern "C" void ddsctx_send(
    const dds_domainid_t domainid,
    const char* topic,
    void* data
)   { DDS::send(domainid, topic, data); }
extern "C" void ddsctx_read(
    const dds_domainid_t domainid,
    const char* topic,
    const int sample
)   { return DDS::read(domainid, topic, sample); }
extern "C" void ddsctx_take(
    const dds_domainid_t domainid,
    const char* topic,
    const int sample
)   { return DDS::take(domainid, topic, sample); }
extern "C" void ddsctx_set_topic_callback(
    const dds_domainid_t domainid,
    const char* topic,
    ddsctx_callback_t callback
)   { DDS::set_topic_callback(domainid, topic, callback); }
extern "C" void ddsctx_set_reader_callback(
    const dds_domainid_t domainid,
    const char* topic,
    ddsctx_callback_t callback
)   { DDS::set_reader_callback(domainid, topic, callback); }
extern "C" void ddsctx_set_writer_callback(
    const dds_domainid_t domainid,
    const char* topic,
    ddsctx_callback_t callback
)   { DDS::set_writer_callback(domainid, topic, callback); }
extern "C" void* ddsctx_get_data(const int sample)
    { return DDS::get_data(sample); }
extern "C" int ddsctx_get_valid(const int sample)
    { return DDS::get_valid(sample); }

#endif//__DDSCTX_OBJECT

#endif//__DDSCTX_HPP