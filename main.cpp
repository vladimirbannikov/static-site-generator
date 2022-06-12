#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
std::string convert_line_to_html(const std::string& gmi_line){ //translation of one line from gmi to html
    std::string html_line;
    static std::vector<std::pair<std::regex,std::string>> dictionary; //translation dictionary
    if(dictionary.empty()) {
        dictionary.emplace_back(std::regex(R"(^# (.*))"), "h1");
        dictionary.emplace_back(std::regex(R"(^## (.*))"), "h2");
        dictionary.emplace_back(std::regex(R"(^### (.*))"), "h3");
        dictionary.emplace_back(std::regex(R"(^\* (.*))"), "li");
        dictionary.emplace_back(std::regex(R"(^> (.*))"), "blockquote");
        dictionary.emplace_back(std::regex(R"(^=>\s*(\S+)(\s+.*)?)"), "a");
    }
    std::string tag, inner_text, href;
    std::smatch mh;
    for(const auto& match:dictionary){ //search for matches of a string with a dictionary
        if (std::regex_match(gmi_line, mh, match.first)){ //if there are matches, the necessary tags are added
            tag = match.second;
            if(tag == "a"){
                inner_text = (mh.size()>1)? mh.str(2):"";
                std::string::iterator it = inner_text.begin();
                while(*it==' ') it++;
                inner_text.erase(inner_text.begin(),it);
                href = mh.str(1);
                html_line = "<" + tag + " href=\"" + href + "\">" + inner_text + "</" + tag + "><br>";
                return html_line;
            }
            else{
                inner_text = mh.str(1);
                html_line = "<" + tag + ">" + inner_text + "</" + tag + ">";
                return html_line;
            }
        }
    }
    html_line = "<p>" + gmi_line + "</p>"; //no matches
    return html_line;
}
bool endsWith(std::string const &str, std::string const &suffix) noexcept{ //function to check if a string ends with a substring
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.rfind(suffix) == str.size() - suffix.size();
}
int gmi_file_to_html(std::ifstream& gmi, std::ofstream& html){ //convert gmi file to html
    html << "<!DOCTYPE HTML>\n";
    html << "<html>\n";
    bool preformat = false;
    bool in_list = false;
    std::string s;
    while(getline(gmi, s)){
        if(!s.empty()){
            if(s=="```"){ //if preformat, then the following lines are translated unchanged
                preformat = !preformat;
                std::string repl = preformat? "<pre>":"</pre>";
                html << std::regex_replace(s,std::regex(R"(```)"),repl);
            }
            else if(preformat){
                html << s;
            }
            else{ //if not preformat, strings are modified
                std::string html_line;
                html_line = convert_line_to_html(s);
                if(html_line.rfind("<li>", 0) == 0){
                    if(!in_list){
                        in_list = true;
                        html << "<ul>\n";
                    }
                    html << html_line;
                }
                else if(in_list){
                    in_list = false;
                    html << "</ul>\n" << html_line;
                }
                else html << html_line;
            }
        }
        else html << "<br>";
        html << "\n";
    }
    html << "</html>\n";
    if (preformat) return 1;
    return 0;
}
void run_trough_directory(const std::string& in_path, const std::string& out_path) { //function that loops through a directory and looks for gmi files
    std::string new_path;
    std::string old_ext = ".gmi";
    std::string new_ext = ".html";
    for (auto &p: fs::recursive_directory_iterator(in_path)) {
        const fs::path &old_path = p.path();
        new_path = old_path.string();
        new_path.replace(new_path.begin(), new_path.begin() + in_path.size(), out_path); // creating directory copy path
        if (fs::is_directory(old_path)) { // if directory
            if (fs::exists(new_path)) {
                std::cout << "\x1b[33mwarning: directory " << new_path << " already exists\x1b[0m"
                          << std::endl;
            } else {
                try{
                    fs::create_directory(new_path);
                }
                catch(fs::filesystem_error &e){
                    std::cout << "\x1b[31m" << e.code().message() << " "<< e.path1() << "\x1b[0m" << '\n';
                }
            }
        } else { // if file
            if (endsWith(new_path, old_ext)) { //if gmi file
                new_path.replace(new_path.end() - old_ext.size(), new_path.end(), new_ext); //creating html file path
                if (fs::exists(new_path)) {
                    std::cout << "\x1b[33mwarning: file " << new_path << " already exists\x1b[0m"
                              << std::endl;
                } else {
                    try{
                        std::ifstream gmi_file(old_path);
                        std::ofstream html_file(new_path);
                        int result = gmi_file_to_html(gmi_file, html_file); // creating html file
                        if (result == 1) {
                            std::cout << "\x1b[33mWarning: Missing closing ``` in file " <<
                                      old_path.string() << "\x1b[0m" << std::endl;
                        }
                    }
                    catch(std::ifstream::failure& e){
                        std::cout << "\x1b[31m" << e.what() << "\x1b[0m" << '\n';
                    }
                }
            } else if (fs::exists(new_path)) {
                std::cout << "\x1b[33mwarning: file " << new_path << " already exists\x1b[0m"
                          << std::endl;
            } else { // if not gmi, just copy
                try{
                    fs::copy_file(old_path, new_path);
                }
                catch (fs::filesystem_error &e) {
                    std::cout << "\x1b[31m" << e.code().message() << " " << e.path1() << "\x1b[0m" << '\n';
                }
            }
        }
    }
}


int main(){
    std::string in_path;
    std::string out_path;
    bool fl_for_start = false;
    while(!fl_for_start) {
        std::cout << "enter an input directory\n> ";
        getline(std::cin, in_path);
        if(!(fs::exists(in_path))) {
            std::cout << "\x1b[31merror: directory/file " << in_path << " does not exist\x1b[0m"
                      << std::endl;
        } else fl_for_start = true;
    }
    fl_for_start = false;
    while(!fl_for_start) {
        std::cout << "enter an output directory\n> ";
        getline(std::cin, out_path);
        if (fs::exists(out_path)) {
            std::cout << "\x1b[33mwarning: directory " << out_path << " already exists\x1b[0m"
                      << std::endl;
            fl_for_start = true;
        } else {
            try {
                fs::create_directories(out_path);
                fl_for_start = true;
            }
            catch (fs::filesystem_error &e) {
                std::cout << "\x1b[31m" << e.code().message() << " "<< e.path1() << "\x1b[0m" << '\n';
            }
        }
    }
    run_trough_directory(in_path, out_path);
    std::cout << "success!" << std::endl;
    return 0;
}
