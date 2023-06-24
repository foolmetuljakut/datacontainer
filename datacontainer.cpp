#include <iostream>
#include <variant>
#include <vector>
#include <map>
#include <exception>

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

    std::string access(std::string ikey = "", std::string separator = ".") {
        // ikey can be of the form 
        // "path/to/0/variable"
        // => split accordingly by separator, 
        // access the first element,
        // cast it if necessary -> key
        // access the data by this key
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
            return std::get<list>(container)[(size_t)atoi(nkey.c_str())].access(rkey, separator); 
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
            return d[nkey].access(rkey, separator); 
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

            }
            else if(container.index() == 1) { 
                if(!isnumber(nkey))
                    throw "what do you expect me to do, access a list using some kind of string?";
                list l = std::get<list>(container);
                std::get<list>(container)[(size_t)atoi(nkey.c_str())].set(rkey, value, separator);
            }
            if(container.index() == 2) {
                dict d = std::get<dict>(container);
                if(d.find(nkey) == d.end())
                    d[nkey] = DataContainer();
                d[nkey].set(rkey, value, separator);
                container = d;
            }
        }
    }
    void set(std::string key, DataContainer& value) {
        if(container.index() == 2)
            std::get<dict>(container)[key] = value;
    }
    friend std::ostream& operator<<(std::ostream& out, DataContainer& dc);
};

std::ostream& operator<<(std::ostream& out, DataContainer& dc) {
    auto container = dc.container;
    switch(container.index()) {
        case 0: // std::string
            return out << std::get<DataContainer::data>(container);
        case 1: // std::vector
            out << "[\n";
            for(auto& item : std::get<DataContainer::list>(container))
                out << item << std::endl;
            return out << "]";
        case 2: // std::map
            out << "{\n";
            for(auto& kvpair : std::get<DataContainer::dict>(container))
                out << kvpair.first << ": " << kvpair.second << std::endl;
            return out << "}";
        default: // ???
            throw "the fuck did you even do?!?";
        }
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

    std::cout << "whats under the first key \"outerkey.key2\":\n"  << containingSample.access("outerkey.key2") << std::endl << "-----------------" << std::endl;
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

    std::cout << "whats behind 1.woodlenoodle? " << listtest.access("1.woodlenoodle") << std::endl;
}

int main(int argc, char **argv) {

    /*todo
        access method for setting values
        json export
        json import
    */

    DataContainer sample;
    sample.set("lol.rofl", "val");
    std::cout << sample << std::endl;

    DataContainer sample;
    sample.set("lol.3", "rofl");
    std::cout << sample << std::endl;

    DataContainer sample(1);
    sample.set("3.lol", "rofl");
    std::cout << sample << std::endl;
    return 0;
}