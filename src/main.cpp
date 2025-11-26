#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <thread>
#include <filesystem>
#include <memory>
#include <gtkmm-4.0/gtkmm.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

void check_layer_shell_support()
{
    bool supported = gtk_layer_is_supported();
    
    if(!supported)
    {
        std::cout << "gtk-layer-shell protocol is not supported on your wayland WM" << std::endl;
        std::exit(0);
    }
}

void chdir_to_parentpath()
{
    std::string path;
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        path = buffer;
    }
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    chdir(dir.string().c_str());
}

void check_wayland_support()
{
    if (!(strcmp(std::getenv("XDG_SESSION_TYPE"), "wayland") == 0))
    {
        std::cout << "This application works only on wayland" << std::endl;
        std::exit(0);
    }
}

std::filesystem::path getFilePath(const std::string& file)
{
    if (std::filesystem::exists(Glib::get_home_dir() + "/.config/GTKAppmenu/" + file))
        return (Glib::get_home_dir() + "/.config/GTKAppmenu/" + file);
    else if (std::filesystem::exists("./" + file))
        return ("./" + file);

    std::cout << "Could not find file: " << file << std::endl;
    std::exit(-1);
}

void check_conf_dir()
{
    if (!std::filesystem::exists(Glib::get_home_dir() + "/.config/GTKAppmenu"))
    {
        std::cout << "\n~/.config/GTKAppmenu doesn't exist.\ncreate it and move conf directory into it\nto use this Programm without the project files i.e. executable only\n"<< std::endl;
    }

    if(!std::filesystem::exists("./conf") && !std::filesystem::exists(Glib::get_home_dir() + "/.config/GTKAppmenu/conf"))
    {
        std::cout << "\nCouldn't find conf directory \nmake sure it exists either in workingDir or in ~/.config/GTKDock" << std::endl;
        std::exit(0);
    }
}

void GLS_setup_layer(Gtk::Window * win, int dispIdx, const std::string& name)
{
    gtk_layer_init_for_window(GTK_WINDOW(win->gobj()));
    gtk_layer_set_layer(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_namespace(GTK_WINDOW(win->gobj()), name.c_str());
    gtk_layer_set_monitor(GTK_WINDOW(win->gobj()), GDK_MONITOR(Gdk::Display::get_default()->get_monitors()->get_object(dispIdx)->gobj()));
    gtk_layer_set_keyboard_mode(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
}

void GLS_setup_fullscreen(Gtk::Window * win)
{
    gtk_layer_set_anchor(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_EDGE_RIGHT, true);
    gtk_layer_set_anchor(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_EDGE_LEFT, true);
    gtk_layer_set_anchor(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_EDGE_TOP, true);
    gtk_layer_set_anchor(GTK_WINDOW(win->gobj()), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
}

std::string to_lower(const std::string& s) {
    std::string result;
    std::transform(s.begin(), s.end(), std::back_inserter(result),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

struct AppEntry
{
    std::string name = "";
    std::string execCmd = "";
    std::string iconPath = "";
    std::string desktopFile = "";

    bool operator<(AppEntry& other) const {
        if (to_lower(name) == to_lower(other.name))
            return to_lower(execCmd) < to_lower(other.execCmd);
        return to_lower(name) < to_lower(other.name);
    }

    bool operator==(AppEntry& other) const {
        return (name == other.name && execCmd == other.execCmd);
    }
};

std::vector<std::string> splitStr(std::string str, std::string separator)
{
    std::vector<std::string> result;
    size_t pos = 0;
    size_t separatorLength = separator.length();

    if (separatorLength == 0) {
        // If separator is empty, return the original string as single element
        result.push_back(str);
        return result;
    }

    while ((pos = str.find(separator)) != std::string::npos) {
        result.push_back(str.substr(0, pos));
        str.erase(0, pos + separatorLength);
    }

    // Add the remaining part of the string
    if (!str.empty()) {
        result.push_back(str);
    }

    return result;
}

std::vector<std::filesystem::path> getDesktopFileSearchPaths()
{
	const char * XDG_DATA_HOME = getenv("XDG_DATA_HOME");
	const char * XDG_DATA_DIRS = getenv("XDG_DATA_DIRS");

    // Standard locations for desktop files
    std::vector<std::filesystem::path> searchPaths = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        Glib::get_home_dir() + "/.local/share/applications",
        Glib::get_home_dir() + "/.local/share/flatpak/exports/share/applications",
        "/var/lib/flatpak/exports/share/applications"
    };
    
    if (XDG_DATA_HOME != NULL)
    {
		searchPaths.push_back(std::string(XDG_DATA_HOME) + "/applications");
	}

    if (XDG_DATA_DIRS != NULL)
    {
        for (std::string& dir : splitStr(XDG_DATA_DIRS, ":"))
        {
            if (dir[dir.size()-1] == '/') searchPaths.push_back(dir + "applications");
            else searchPaths.push_back(dir + "/applications");
        }
    }

    return searchPaths;
}

std::vector<std::filesystem::path> findDesktopFiles() {
    std::vector<std::filesystem::path> desktopFiles;

    for (const auto& path : getDesktopFileSearchPaths()) {
        if (std::filesystem::exists(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.path().extension() == ".desktop") {
                    desktopFiles.push_back(entry.path());
                }
            }
        }
    }
    
    return desktopFiles;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

std::string cleanExecCommand(const std::string& cmd) {
    std::string result = cmd;

    result = trim(result);
    return result;
}

std::string findIconPath(const std::string& iconName) {
    // Check if it's already an absolute path
    if (std::filesystem::exists(iconName)) {
        return iconName;
    }

    // Try theme icons first
    try {
        auto iconTheme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
        auto iconInfo = iconTheme->lookup_icon(iconName, 48);
        return iconInfo->get_file()->get_path();
    } catch (...) {
        // Fallback to common paths
        std::vector<std::string> extensions = {".png", ".svg", ".xpm"};
        std::vector<std::filesystem::path> searchPaths = {
            "/usr/share/pixmaps",
            "/usr/share/icons/hicolor/48x48/apps",
            "/usr/share/icons/hicolor/scalable/apps",
            "/usr/share/icons/Adwaita/48x48/apps",
            "/usr/share/icons"
        };

        for (const auto& path : searchPaths) {
            if (!std::filesystem::exists(path)) continue;
            
            for (const auto& ext : extensions) {
                std::filesystem::path iconPath = path / (iconName + ext);
                if (std::filesystem::exists(iconPath)) {
                    return iconPath.string();
                }
            }
        }
    }
    return "";
}

AppEntry parseDesktopFile(const std::filesystem::path& desktopFile) {
    AppEntry entry;
    
    std::ifstream file(desktopFile);
    std::string line;
    bool mainSection = false;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line == "[Desktop Entry]") {
            mainSection = true;
            continue;
        } else if (line[0] == '[') {
            mainSection = false;
            continue;
        }
        
        if (!mainSection) continue;
        
        size_t delim = line.find('=');
        if (delim == std::string::npos) continue;
        
        std::string key = line.substr(0, delim);
        std::string value = line.substr(delim + 1);
        
        if (key == "Name") {
            entry.name = value;
        } else if (key == "Exec") {
            entry.execCmd = cleanExecCommand(value);
        } else if (key == "Icon") {
            entry.iconPath = findIconPath(value);
        }
    }
    
    return entry;
}

std::string exec(const std::string& command)
{
    char buffer[256];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "popen failed!";
    }
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    int status = pclose(pipe);
    if (status == -1) {
        return "Error closing the pipe!";
    }
    return result;
}

// ugly C function
char *readLine(FILE *stream)
{
    char *buffer = NULL;
    size_t size = 0;
    char *newline_found = NULL;
 
    do
    {
        char *temp = (char *)realloc(buffer, size + BUFSIZ);
        if (!temp)
        {
            free(buffer);
            return NULL;
        }
        buffer = temp;
 
        if (!fgets(buffer + size, BUFSIZ, stream))
        {
            free(buffer);
            return NULL;
        }
 
        newline_found = strchr(buffer + size, '\n');
    } while (!newline_found && (size += BUFSIZ - 1));
 
    return buffer;
}

std::string normalizeString(const std::string& input) {
    std::string result;
    
    for (char c : input) {
        // Convert to lowercase
        char lower = tolower(c);
        
        // Keep only alphanumeric characters
        if (isalnum(lower)) {
            result += lower;
        }
    }
    
    return result;
}

bool find_case_insensitive(const std::string& str, const std::string& substr) {
    std::string lower_str = normalizeString(str);
    std::string lower_sub = normalizeString(substr);

    return (lower_str.find(lower_sub) != std::string::npos);
}

std::string getSmallestString(const std::vector<std::string>& strings)
{
    int ind = 0;
    int size = 0;
    
    for (int i = 0; i < strings.size(); i++)
    {
        std::string str = strings[i];
        if (str.size() > size)
        {
            ind = i;
            size = str.size();
        }
    }

    return strings[ind];
}

class Win : public Gtk::Window 
{
    public:
        std::vector <AppEntry> apps;

        Win()
        {
            GLS_setup_layer(this, 1, "Panel");
            GLS_setup_fullscreen(this);

            auto ev = Gtk::EventControllerKey::create();

            ev->signal_key_pressed().connect([this](guint keyval, guint keycode, Gdk::ModifierType state) mutable {
                if (keyval == GDK_KEY_Escape) this->get_application()->quit();
                return true;
            }, false);

            add_controller(ev);

            auto files = findDesktopFiles();
            for (auto& file : files)
            {
                apps.emplace_back(parseDesktopFile(file));
            }

            std::sort(apps.begin(), apps.end());
            apps.erase(std::unique(apps.begin(), apps.end()), apps.end());
        
            buildWindow();
        }
    private:
        Gtk::Entry* searchEntry = nullptr;
        Gtk::FlowBox* flow = nullptr;

        void buildWindow()
        {
            // Main vertical layout: Search bar + scroll area
            auto vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
            vbox->set_margin(10);
            vbox->set_margin_start(300);
            vbox->set_margin_end(300);

            // --- SEARCH BAR ---
            searchEntry = Gtk::make_managed<Gtk::Entry>();
            searchEntry->set_placeholder_text("Search applications…");
            vbox->append(*searchEntry);

            // Scroll container
            auto scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
            scroller->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
            scroller->set_hexpand(true);
            scroller->set_vexpand(true);
            vbox->append(*scroller);

            // FlowBox for dynamic grid
            flow = Gtk::make_managed<Gtk::FlowBox>();
            flow->set_selection_mode(Gtk::SelectionMode::NONE);
            flow->set_valign(Gtk::Align::START);
            //flow->set_halign(Gtk::Align::START);
            flow->set_column_spacing(20);
            flow->set_row_spacing(20);
            flow->set_margin(20);

            scroller->set_child(*flow);

            // Fill initial display
            rebuildFlow("");

            // Search updates
            searchEntry->signal_changed().connect([this]() {
                rebuildFlow(searchEntry->get_text());
            });

            this->set_child(*vbox);
        }

    void rebuildFlow(const std::string& query)
    {
        flow->remove_all();

        for (const auto& app : apps)
        {
            if (!query.empty())
            {
                if (!find_case_insensitive(app.name, query))
                    continue;
            }

            // Icon
            Gtk::Image* img = nullptr;
            try { img = Gtk::make_managed<Gtk::Image>(app.iconPath); }
            catch (...) { img = Gtk::make_managed<Gtk::Image>(); }
            img->set_pixel_size(48);

            // Label
            auto lbl = Gtk::make_managed<Gtk::Label>(app.name);
            lbl->set_ellipsize(Pango::EllipsizeMode::END);
            lbl->set_lines(2);
            lbl->set_max_width_chars(10);
            lbl->set_justify(Gtk::Justification::CENTER);
            
            auto b = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
            b->append(*img);
            b->append(*lbl);
            b->set_margin(10);

            auto box = Gtk::make_managed<Gtk::CenterBox>();
            box->set_size_request(100,100);
            box->set_orientation(Gtk::Orientation::VERTICAL);
            box->add_css_class("app");
            
            box->set_center_widget(*b);
            
            // Event controller for click
            auto click = Gtk::GestureClick::create();
            click->signal_released().connect([this, app](int, double, double) {
                std::cout << (app.execCmd) << std::endl;
                this->get_application()->quit();
            });
            box->add_controller(click);

            flow->append(*box);
        }
    }
};

int main(int argc, char **argv)
{
    chdir_to_parentpath();
    check_conf_dir();
    check_wayland_support();
    check_layer_shell_support();

    auto app = Gtk::Application::create();
   
    app->signal_startup().connect([app, argc, argv](){
        auto win = Gtk::make_managed<Win>();
        
        Glib::RefPtr<Gtk::CssProvider> css_provider = Gtk::CssProvider::create();
        css_provider->load_from_path(getFilePath("conf/style.css"));
            
        win->get_style_context()->add_provider_for_display(Gdk::Display::get_default(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
        app->add_window(*win);
        win->present();
    });

    return app->run();
}
