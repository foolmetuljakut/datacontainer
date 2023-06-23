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

        unsigned i = ikey.find_first_of(separator);
        std::string nkey = ikey.substr(0, i);
        std::string rkey = ikey.substr(i + 1);

        std::cout << "access-print: \n" << *this << std::endl;

        switch(container.index()) {
        case 0: // std::string
            return std::get<data>(container);
        case 1: // std::vector
            if(ikey.size() == 0)
                throw "";
            if(rkey.size() == 0) 
                throw "not implemented: special case";
            return std::get<list>(container)[(unsigned)atoi(nkey.c_str())].access(rkey);
        case 2: // std::map
            if(!nkey.compare(rkey)) { // special case: end of path and last element is a map itself
                auto inner = std::get<dict>(container)[nkey];
                std::cout << inner << std::endl;
                //wieso entsteht hier eine rekursion????
                return std::get<data>(inner.container); //std::get<data>(inner);
            }
            return std::get<dict>(container)[nkey].access(nkey.compare(rkey) ? rkey : "");
        default: // ???
            throw "the fuck did you even do?!?";
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
private:
    dctype container;
};

std::ostream& operator<<(std::ostream& out, DataContainer& dc) {
    auto container = dc.container;
    out << "container type: ";
    switch(container.index()) {
        case 0: // std::string
            return out << "data, " << std::get<DataContainer::data>(container);
        case 1: // std::vector
            out << "list\n";
            for(auto& item : std::get<DataContainer::list>(container))
                out << item << std::endl;
            return out;
        case 2: // std::map
            out << "dict\n";
            for(auto& kvpair : std::get<DataContainer::dict>(container))
                out << kvpair.first << ": " << kvpair.second << std::endl;
            return out;
        default: // ???
            throw "the fuck did you even do?!?";
        }
}

int main(int argc, char **argv) {

    // DataContainer sample("value");
    DataContainer sample;
    sample.set("key", "value");
    sample.set("key2", "value");
    DataContainer containingSample;
    containingSample.set("outerkey", sample);
    std::cout << sample.access("outerkey.key2") << std::endl;
    return 0;
}