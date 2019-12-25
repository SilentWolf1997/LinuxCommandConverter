#ifndef __BUFFER_HH__
#define __BUFFER_HH__

template <class _data_type>
class buffer {
private:
    _data_type* pdata;
public:
    template <class ...Args>
    buffer<_data_type>(Args ...args) {
        pdata = new _data_type(args...);
    }

    buffer<_data_type>(const buffer<_data_type>& other) {
        pdata = new _data_type(*(other.pdata));
    }

    buffer<_data_type>(buffer<_data_type>&& other) {
        pdata = other.pdata;
        other.pdata = nullptr;
    }

    ~buffer<_data_type>() {
        if (pdata) {
            delete pdata;
        }
    }

    buffer<_data_type>& operator=(const buffer<_data_type>& other) {
        delete pdata;
        pdata = new _data_type(*(other.pdata));
        return *this;
    }

    buffer<_data_type>& operator=(buffer<_data_type>&& other) {
        delete pdata;
        pdata = other.pdata;
        other.pdata = nullptr;
        return *this;
    }

    _data_type* operator->() {
        return pdata;
    }

    operator void*() {
        return (void *)pdata;
    }
};

#endif
