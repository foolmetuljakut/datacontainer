#include <iostream>
#include <sstream>
#include <variant>
#include <vector>
#include <map>
#include <exception>

#include <boost/asio.hpp>
#include <string>
using namespace boost::asio;
using ip::tcp;

class DataContainer {
public:
    typedef std::string data;
    typedef std::vector<DataContainer> list;
    typedef std::map<data, DataContainer> dict;
    typedef std::variant<data, list, dict> dctype;
    typedef std::variant<size_t, std::string> keytype;
private:
    dctype container;
    bool isnumber(const std::string s) { return s.find_first_not_of( "0123456789" ) == std::string::npos; }
    size_t filllist(list& l, std::string indexstr) {
        size_t index = (size_t)atoi(indexstr.c_str()),
            size = l.size();
        if(index >= size)
            for(unsigned tmpi = 0; tmpi < index + 1 - size; tmpi++)
                l.push_back(DataContainer());
        return index;
    }
public:
    DataContainer(size_t containerindex = 2) {
        switch(containerindex) {
            case 0:
                container = data();
                return;
            case 1:
                container = list();
                return;
            case 2:
                container = dict();
                return;
        }
    }
    DataContainer(data value) : container(value) {}
    DataContainer(std::initializer_list<data> list) {
        std::vector<DataContainer> dc;
        dc.reserve(list.size());
        for(auto& s : list)
            dc.emplace_back(DataContainer(s));
        container = std::move(dc);
    }
    DataContainer(std::initializer_list<DataContainer> list) {
        std::vector<DataContainer> dc;
        for(auto& s : list)
            dc.push_back(s);
        container = std::move(dc);
    }

    std::string get(std::string ikey = "", std::string separator = ".") {
        // ikey can be of the form 
        // "path/to/0/variable"
        // => split accordingly by separator, 
        // get the first element,
        // cast it if necessary -> key
        // get the data by this key
        // pass on the rest of the ikey -> rkey

        if(container.index() == 0)
            return std::get<data>(container);

        // index > 0 => we must have some kind of accessor
        // accessor is numeric => lists
        // accessor is string => dict

        size_t i = ikey.find_first_of(separator);
        std::string nkey, rkey;
        if(i < (size_t)(-1)) {
            nkey = ikey.substr(0, i);
            rkey = ikey.substr(i + 1);
        }
        else { // no separator => last element before value
            nkey = ikey;
            rkey = "\0";
        }
        
        // only two remaining options
        if(container.index() == 1) { // list
            if(ikey.size() == 0)
                throw "";
            if(rkey.size() == 0)
                throw "not implemented: special case";
            if(!isnumber(nkey))
                throw "what do you expect me to do, access a list using some kind of string?";
            return std::get<list>(container)[(size_t)atoi(nkey.c_str())].get(rkey, separator); 
        } else { //dict
            dict d = std::get<dict>(container);
            if(d.find(nkey) == d.end())
                throw "gibt's nicht! kauf ich auch nicht!";
            if(!rkey.compare("\0")) { // reached end of key - if this isn't a value we don't have enough keys
                auto lastc = d[nkey].container;
                if(lastc.index() > 0)
                    throw "accessed element is not a value. key is to short to go all the way down.";
                return std::get<data>(lastc);
            }
            return d[nkey].get(rkey, separator); 
        } 
    }

    void set(std::string key, std::string value, std::string separator = ".") {
        size_t i = key.find_first_of(separator);
        std::string nkey, rkey;
        if(i < (size_t)(-1)) {
            nkey = key.substr(0, i);
            rkey = key.substr(i + 1);
        }
        else { // no separator => last element before value
            nkey = key;
            rkey = "\0";
        }

        if(!rkey.compare("\0")) {
            DataContainer v(value);
            set(nkey, v); 
        }
        else {
            if(container.index() == 0) {
                throw "reached a value, can't follow path.";
            }
            else if(container.index() == 1) { 
                if(!isnumber(nkey))
                    throw "what do you expect me to do, access a list using some kind of string?";
                list& l = std::get<list>(container);
                l[filllist(l, nkey)].set(rkey, value, separator);
            }
            if(container.index() == 2) {
                dict& d = std::get<dict>(container);
                if(d.find(nkey) == d.end())
                    d[nkey] = DataContainer(isnumber(rkey) ? 1 : 2);
                d[nkey].set(rkey, value, separator);
                container = d;
            }
        }
    }
    void set(std::string key, DataContainer& value) {
        if(container.index() == 1) {
            list& l = std::get<list>(container);
            l[filllist(l, key)] = value;
        } 
        else if(container.index() == 2)
            std::get<dict>(container)[key] = value;
    }
    
    std::string json(int level = 1, std::string indent = "    ") { 
        std::stringstream ss;
        if(container.index() == 0) {
            ss << "\"" << std::get<data>(container) << "\"";
        }
        else if (container.index() == 1)
        {
            ss << "[\n";
            size_t oi = 0;
            auto d = std::get<list>(container);
            for(auto& o : d) {
                for(int i{0}; i < level; i++)
                    ss << indent;
                ss << o.json(level + 1) << ( oi++ < d.size() -1 ? ", " : "") << "\n";
            } 
            for(int i{0}; i < level-1; i++)
                ss << indent; 
            ss << "]\n";
        }
        else if(container.index() == 2) {
            ss << "{\n";
            size_t oi = 0;
            auto d = std::get<dict>(container);
            for(auto& o : d) {
                for(int i{0}; i < level; i++)
                    ss << indent;
                ss << "\"" << o.first << "\"" << ": " << o.second.json(level + 1) << 
                    ( oi++ < d.size() -1 ? ", " : "") << std::endl;
            }
            for(int i{0}; i < level-1; i++)
                ss << indent; 
            ss << "}";
        }
        return ss.str();
    }
    friend std::ostream& operator<<(std::ostream& out, DataContainer& dc);
};

std::ostream& operator<<(std::ostream& out, DataContainer& dc) {
    return out << dc.json();
}

// testing value and dict access
void main0() {

    DataContainer sample("value");
    std::cout << "sample with just value:\n" << sample << std::endl << "-----------------" << std::endl;

    sample = DataContainer();
    sample.set("key", "value");
    sample.set("key2", "value");
    std::cout << "sample with 2 key value pairs:\n"  << sample << std::endl << "-----------------" << std::endl;

    DataContainer containingSample;
    containingSample.set("outerkey", sample);
    std::cout << "sample with sub sample:\n"  << containingSample << std::endl << "-----------------" << std::endl;

    std::cout << "whats under the first key \"outerkey.key2\":\n"  << containingSample.get("outerkey.key2") << std::endl << "-----------------" << std::endl;
}

void main1() {

    DataContainer listtest({"1", "2", "3", "4"});
    std::cout << listtest << std::endl;

    DataContainer s1("value1"), s2("value2");
    listtest = DataContainer({s1, s2});
    std::cout << listtest << std::endl;

    DataContainer containingSample1, containingSample2;
    containingSample1.set("woodadooda", s1);
    containingSample2.set("woodlenoodle", s2);
    listtest = DataContainer({containingSample1, containingSample2});
    std::cout << listtest << std::endl;

    std::cout << "whats behind 1.woodlenoodle? " << listtest.get("1.woodlenoodle") << std::endl;
}

void main2() {

    DataContainer sample;
    sample.set("lol.rofl", "val");
    std::cout << sample << std::endl;

    sample = DataContainer();
    sample.set("lol.3", "rofl");
    sample.set("lol.1", "rofl2");
    std::cout << sample << std::endl;

    sample = DataContainer(1);
    sample.set("3.lol", "rofl");
    std::cout << sample << std::endl;
}

    /*todo
        json import
    */

std::string read(tcp::socket& socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\0");
    std::string data = boost::asio::buffer_cast<const char*>(buf.data());
    return data;
}

void send(tcp::socket& socket, const std::string& message) {
    const auto msg = message + "\0";
    boost::asio::write(socket, boost::asio::buffer(msg));
}

void main3(int argc, char **argv) {

    if(std::string(argv[1]) == "send") {
        boost::asio::io_service service;
        tcp::socket socket(service);
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
        
        std::string msg(argv[2]);
        send(socket, msg);
    }
    else if(std::string(argv[1]) == "receive") {
        boost::asio::io_service service;
        tcp::acceptor acceptor(service, tcp::endpoint(tcp::v4(), 1234));
        tcp::socket socket(service);
        acceptor.accept(socket);
        std::cout << "received: >" << read(socket) << "<" << std::endl;
    }
}


int main(int argc, char **argv) {

#ifdef MAIN
    main3(argc, argv);
#endif
#ifdef UT1
    main0();
#endif
#ifdef UT2
    main1();
#endif
#ifdef UT3
    main2();
#endif

    return 0;
}