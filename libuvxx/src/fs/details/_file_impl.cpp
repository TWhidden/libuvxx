#include <fcntl.h>
#include "fs/details/_file_impl.hpp"

namespace uvxx { namespace fs { namespace details
{
    static int get_open_flags(std::ios_base::openmode mode)
    {
        int result = 0;
        if (mode & std::ios_base::in)
        {
            if (mode & std::ios_base::out)
            {
                result = O_RDWR;
            }
            else
            {
                result = O_RDONLY;
            }
        }
        else if (mode & std::ios_base::out)
        {
            result = O_WRONLY | O_CREAT;
        }

        if (mode & std::ios_base::app)
        {
            result |= O_APPEND;
        }

        if (mode & std::ios_base::trunc)
        {
            result |= O_TRUNC | O_CREAT;
        }

        return result;
    }

    void _file_impl::open_callback(int exception_code)
    {
        if (exception_code)
        {
            _open_event.set_exception(uvxx::uv_exception_with_code(exception_code));
        }
        else
        {
            _open_event.set();
        }
    }

    void _file_impl::read_callback(const uint8_t* buf, int len, int exception_code)
    {
        if (exception_code)
        {
            _read_event.set_exception(uvxx::uv_exception_with_code(exception_code));
        }
        else
        {
            _file_position += len;
            _read_event.set(len);
        }
    }

    void _file_impl::write_callback(const uint8_t* buf, int len, int exception_code)
    {
        if (exception_code)
        {
            _write_event.set_exception(uvxx::uv_exception_with_code(exception_code));
        }
        else
        {
            _file_position += len;
            _write_event.set(len);
        }
    }

    void _file_impl::close_callback(int exception_code)
    {
        if (exception_code)
        {

            _close_event.set_exception(uvxx::uv_exception_with_code(exception_code));
        }
        else
        {
            _close_event.set();
        }
    }

    _file_impl::_file_impl() : _file(&dispatcher()->_loop), 
                               _task_context(uvxx::pplx::task_continuation_context::use_current())
    {
        auto read_cb = std::bind(&_file_impl::read_callback,
                                 this,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::placeholders::_3);

        auto write_cb = std::bind(&_file_impl::write_callback,
                                this,
                                std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3);

        auto open_cb = std::bind(&_file_impl::open_callback,
                                 this,
                                 std::placeholders::_1);

        auto close_cb = std::bind(&_file_impl::close_callback,
                                  this,
                                  std::placeholders::_1);

        _file.read_callback_set(read_cb);

        _file.write_callback_set(write_cb);

        _file.close_callback_set(close_cb);

        _file.open_callback_set(open_cb);
    }

    uvxx::pplx::task<void> _file_impl::open_async(std::string const& file_name, std::ios_base::openmode mode)
    {
        verify_access();

        int flags = get_open_flags(mode);

        _file.open(file_name, flags, 0);

        _open_event = uvxx::pplx::task_completion_event<void>();

        return uvxx::pplx::create_task(_open_event, _task_context);
    }

    uvxx::pplx::task<void> _file_impl::close_async()
    {
        verify_access();

        _file.close();

        _close_event = uvxx::pplx::task_completion_event<void>();

        return uvxx::pplx::task<void>(_close_event, _task_context);
    }

    uvxx::pplx::task<int> _file_impl::read_async(io::memory_buffer const& buffer, int start_pos, int count)
    {
        verify_access();

        _file.read(buffer, buffer.length_get(), start_pos, count, _file_position);

        _read_event.reset();

        return uvxx::pplx::create_task(_read_event, _task_context);
    }

    uvxx::pplx::task<int> _file_impl::write_async(io::memory_buffer const& buffer, int start_pos, int count)
    {
         verify_access();
        
         _file.write(buffer, buffer.length_get(), start_pos, count, _file_position);
        
         _write_event.reset();

         return uvxx::pplx::create_task(_write_event, _task_context);
    }

    int64_t _file_impl::file_position_get()
    {
        return _file_position;
    }

    void _file_impl::file_position_set(size_t position)
    {
        verify_access();
        _file_position = position;
    }

}}}