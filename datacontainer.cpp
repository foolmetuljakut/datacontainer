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
    typedef std::variant<unsigned, std::string> keytype;
private:
    dctype container;

public:
    DataContainer(std::string value) : container(value) {}
    DataContainer(std::initializer_list<std::string> list) {
        std::vector<DataContainer> dc;
        dc.reserve(list.size());
        for(auto& s : list)
            dc.emplace_back(DataContainer(s));
        container = std::move(dc);
    }
    DataContainer() : container(std::map<std::string, DataContainer>()) {}
    bool isnumber(const std::string s) { return s.find_first_not_of( "0123456789" ) == std::string::npos; }
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

        unsigned i = ikey.find_first_of(separator);
        std::string nkey, rkey;
        if(i < (unsigned)(-1)) {
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
            return std::get<list>(container)[(unsigned)atoi(nkey.c_str())].access(rkey, separator); 
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
            return std::get<dict>(container)[nkey].access(rkey, separator); 
        } 
    }

    void set(std::string key, std::string value) {
        DataContainer v(value);
        set(key, v);
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

int main(int argc, char **argv) {

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
    return 0;
}