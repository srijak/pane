#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

enum SplitDirection { NONE, VERTICAL, HORIZONTAL };

class Dimensions {
public:
    Dimensions() {}
    Dimensions(int xi, int yi, int widthi, int heighti) {
        x = xi;
        y = yi;
        width = widthi;
        height = heighti;

    }
    int x, y, width, height;
};

class Container;

class ICell {
protected:
    Container* parent_;
    Dimensions d_;
public:
    virtual ~ICell() {}
    virtual void print() =0;
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

    /* balance all cells in this container.
     */
    void balance() {
        // go through children and setup dimensions.
        cout << "Balancing container." << endl;
    }

    virtual void print() {
        cout << "c." << this << " [ ";
        for (vector<ICell*>::iterator it=cells_->begin(); it!=cells_->end(); ++it) {
            (*it)->print();
        }
        cout << " ] ";
    }
    virtual ~Container() {
        delete cells_;
    }
};

class Window: public ICell {
public:
    virtual ~Window() {}
    Window(Dimensions& d) {
        d_ = d;
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

        new_container->balance();
    }

    virtual void print() {
        cout << "w." << this << " " ;
    }
};

int main() {

    string inp = "";
    Dimensions d = Dimensions (0, 0, 200, 400);
    Container* c = new Container(d);
    Window* a = new Window(d);
    Window* b = new Window(d);

    while (inp != "q") {
        cin >> inp;
        if (inp == "sp") {
            cout << "Split window" << endl;
            a->split(VERTICAL, *b);
            c->print();
        } else if (inp == "add") {
            cout << "Add window" << endl;
            c->add_cell(a);
            c->print();
        } else if (inp == "al") {
            Window* e = new Window(d);
            Window* f = new Window(d);
            Window* g = new Window(d);
            c->add_cell(e);
            c->add_cell(f);
            c->add_cell(g);
            c->print();
        }
    }
}

