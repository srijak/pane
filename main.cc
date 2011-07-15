#include <iostream>
#include <vector>
#include <algorithm>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>


using namespace std;

enum SplitDirection { NONE, VERTICAL, HORIZONTAL };

class Dimensions {
public:
    int x, y, width, height;
    Dimensions() {}
    Dimensions(xcb_get_geometry_reply_t* geom) {
        x = geom->x;
        y = geom->y;
        width = geom->width;
        height = geom->height;

    }
    Dimensions(int xi, int yi, int widthi, int heighti) {
        x = xi;
        y = yi;
        width = widthi;
        height = heighti;
    }
};

class Container;

class ICell {
protected:
    Container* parent_;
    Dimensions d_;
public:
    virtual ~ICell() {}
    virtual void print() =0;
    virtual void position(int x, int y, int width, int height) {}
    void set_parent(Container* c) {
        parent_ = c;
    }
    Container* get_parent() {
        return parent_;
    }
};

class Container: public ICell {
    vector<ICell*>* cells_;

public:
    Container(Dimensions& d) {
        d_ = d;
        cout << "Created container with: " << d_.width << "x" << d_.height << endl;
        cells_ = new vector<ICell*>;
    }

    void remove_cell(ICell* const c) {
        // should we unset the parent too? If so what to set it to?
        cells_->erase(remove(cells_->begin(), cells_->end(), c));
    }

    void add_cell(ICell* c) {
        cells_->push_back(c);
        c->set_parent(this);

        cout << "Added cell [" << c << "] to container [" << this << "]" << endl;
        cout << " cells_ has " << cells_->size() << " items" << endl;
    }

    virtual void position(int x, int y, int width, int height) {
        // go through children and setup dimensions.
        // currently just splits them horizontally, can use diff strategies
        // later etc.
        int num_cells = cells_->size();
        if (! num_cells > 0){
          return;
        }
        cout << "Balancing container with "<< num_cells << " cells." << endl;
        int each_height = (height - num_cells)/ num_cells;
        cout << "Each height: " << each_height << endl;

        int start_offset = 0;

        for (vector<ICell*>::iterator it=cells_->begin(); it!=cells_->end(); ++it) {
            cout << " Positioning window at start_offset: " << start_offset << endl;
            (*it)->position(x, start_offset, width, each_height);
            start_offset += each_height + 1;
        }
    }

    virtual void print() {
        cout << "c." << this << " [ ";
        for (vector<ICell*>::iterator it=cells_->begin(); it!=cells_->end(); ++it) {
            (*it)->print();
        }
        cout << " ]";
    }
    virtual ~Container() {
        delete cells_;
    }
};

class Window: public ICell {

    xcb_window_t child_;
    xcb_connection_t* conn_;

public:
    virtual ~Window() {}

    Window(Dimensions& d, xcb_connection_t* conn, xcb_window_t child) {
        d_ = d;
        conn_ = conn;
        child_ = child;
    }
    void position(int x, int y, int width, int height) {

        uint32_t values[4];
        uint32_t mask;
        xcb_void_cookie_t cookie;
        xcb_generic_error_t *error;

        // move and resize client window
        values[0] = x >= 0 ? x : d_.x;
        values[1] = y >= 0 ? y : d_.y;
        values[2] = width > 0 ? width : d_.width;
        values[3] = height > 0 ? height : d_.height;

        cout << "X: " << values[0] << "  Y: " << values[1] << " W: " << values[2] << " H: "<< values[3] << endl;
        mask = XCB_CONFIG_WINDOW_X |XCB_CONFIG_WINDOW_Y |
               XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        cookie = xcb_configure_window_checked(conn_, child_, mask, values);

        error = xcb_request_check(conn_, cookie);
        if(error)
        {
            cout << "Could not resize client." << endl;
            return;
        }

    }

    void split(SplitDirection dir, Window& windowToAdd) {
        // create a new container.
        //   set new container's dimensions to my prev dimensions
        //      (the actual adjusting of the windows diemnsions is done by
        //      the container.
        //   set the split direction
        //   set the split ratio (percentage coevered by each split)
        //   set container's cells to ourself, and windowtoAdd
        // update our container's cell pointer to the new container
        // balance the new container.
        Container* prev_parent = parent_;

        Container* new_container = new Container(d_);
        prev_parent->remove_cell(this);
        prev_parent->add_cell(new_container);

        new_container->add_cell(&windowToAdd);
        new_container->add_cell(this);

        new_container->position(d_.x, d_.y, d_.width, d_.height);
    }

    virtual void print() {
        cout << "w." << this << " " ;
    }

};

class Manager {

private:
    xcb_connection_t* conn_;
    xcb_screen_t* screen_;
    xcb_window_t root_;
    xcb_get_geometry_reply_t* original_geom_;

    Container* root_container_;

    xcb_window_t* get_all_children(int& len) {
        /* Get children*/
        xcb_query_tree_reply_t* reply =  xcb_query_tree_reply(conn_,  xcb_query_tree(conn_, root_), 0);
        if (NULL == reply) {
            cout << "Couldn't get windows" << endl;
        }
        len = xcb_query_tree_children_length(reply);
        xcb_window_t* xchildren = xcb_query_tree_children(reply);
        if (NULL == reply) {
            cout << "Couldn't get reply." << endl;
        }
        free(reply);
        return xchildren;
    }
    void add_existing_windows() {
        int len = 0;
        xcb_window_t* children =  get_all_children(len);

        cout << " have " << len << " windows" << endl;

        for (int i = 0; i < len; i ++) {
            cout << "before add_window: " << children[i] << endl;
            add_window(children[i]);
        }
    }

public:
    void init() {
        int screens;
        conn_ = xcb_connect(NULL, &screens);
        if (xcb_connection_has_error(conn_)) {
            cout << "Error, couldnt connect to xserver" << endl;
        }
        screen_ = xcb_aux_get_screen(conn_, screens);
        root_ = screen_->root;

        Dimensions screen_d = Dimensions (0, 0, screen_->width_in_pixels, screen_->height_in_pixels);
        root_container_ = new Container(screen_d);

        add_existing_windows();
        cout << "Done adding existing windows" << endl;
        root_container_->position(screen_d.x, screen_d.y, screen_d.width, screen_d.height);
    }

    void add_window(xcb_window_t window) {

        cout << "add window " << window << endl;
        xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(conn_, window);

        xcb_drawable_t d = { window };
        xcb_get_window_attributes_reply_t* attr = 0;

        if ((attr = xcb_get_window_attributes_reply(conn_, attr_cookie, 0)) == NULL) {
            cout << "Could not get attributes for Window " << window << endl;
            return;
        }

        if (attr->map_state != XCB_MAP_STATE_VIEWABLE) {
            cout << "not VIEWABLE" << endl;
            return;
        }

        if (attr->override_redirect) {
            cout << "Override_redirect is set: " << attr->override_redirect << endl;
            return;
        }

        xcb_get_geometry_cookie_t geomc = xcb_get_geometry(conn_, d);
        
        if ((original_geom_ = xcb_get_geometry_reply(conn_, geomc, 0)) == NULL) {
            cout << "Couldn't get geomertry." << endl;
            return;
        }

        cout << "Working on window " << window << endl;

        cout << "\t Geom: x: " << original_geom_->x << " y: " << original_geom_->y
             << " width: " << original_geom_->width << " height: " << original_geom_->height << endl
             << "\t Geom->root: " << original_geom_->root << " Geom->depth:" << original_geom_->depth << endl;

        xcb_map_window(conn_, window);
        xcb_flush(conn_);

        Dimensions dim = Dimensions(original_geom_);
        Window* win = new Window(dim, conn_, window);

        root_container_->add_cell(win);
    }

};

int main() {

    string inp = "";
    Manager m = Manager();
    m.init();
    
}

