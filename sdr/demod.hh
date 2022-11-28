#ifndef __SDR_DEMOD_HH__
#define __SDR_DEMOD_HH__

#include "node.hh"
#include "traits.hh"
#include "config.hh"
#include "combine.hh"
#include "logger.hh"
#include "math.hh"
#include <QByteArray>

namespace sdr {


template <class Scalar>
class AMDemod
        : public Sink< std::complex<Scalar> >, public Source
{
public:
    AMDemod() : Sink< std::complex<Scalar> >(), Source()
    {
        // pass...
    }

    virtual ~AMDemod() {
        // free buffers
        _buffer.unref();
    }

    virtual void config(const Config &src_cfg) {
        // Requires type & buffer size
        if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
        // Check if buffer type matches template
        if (Config::typeId< std::complex<Scalar> >() != src_cfg.type()) {
            ConfigError err;
            err << "Can not configure AMDemod: Invalid type " << src_cfg.type()
                << ", expected " << Config::typeId< std::complex<Scalar> >();
            throw err;
        }

        // Unreference previous buffer
        _buffer.unref();
        // Allocate buffer
        _buffer = Buffer<Scalar>(src_cfg.bufferSize());

        LogMessage msg(LOG_DEBUG);
        msg << "Configure AMDemod: " << this << std::endl
            << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
            << " output type: " << Traits<Scalar>::scalarId << std::endl
            << " sample rate: " << src_cfg.sampleRate() << std::endl
            << " buffer size: " << src_cfg.bufferSize();
        Logger::get().log(msg);

        // Propergate config
        this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                               src_cfg.bufferSize(), src_cfg.numBuffers()));
    }

    virtual void process(const Buffer<std::complex<Scalar> > &buffer, bool allow_overwrite)
    {
        Buffer<Scalar> out_buffer;
        // If source allow to overwrite the buffer, use it otherwise rely on own buffer
        if (allow_overwrite) { out_buffer = Buffer<Scalar>(buffer); }
        else { out_buffer = _buffer; }

        // Perform demodulation
        for (size_t i=0; i<buffer.size(); i++) {
            out_buffer[i] = _gain_const * (std::sqrt(buffer[i].real()*buffer[i].real() +
                                                     buffer[i].imag()*buffer[i].imag()));
        }

        // If the source allowed to overwrite the buffer, this source will allow it too.
        // If this source used the internal buffer (_buffer), it allows to overwrite it anyway.
        this->send(out_buffer.head(buffer.size()), true);
    }

protected:
    Buffer<Scalar> _buffer;
    float _gain_const = 2.0;
};


template <class Scalar>
class USBDemod
        : public Sink< std::complex<Scalar> >, public Source
{
public:
    typedef std::complex<Scalar> CScalar;
    typedef typename Traits<Scalar>::SScalar SScalar;

public:
    USBDemod() : Sink<CScalar>(), Source()
    {
        // pass...
    }

    virtual ~USBDemod() {
        _buffer.unref();
    }

    virtual void config(const Config &src_cfg) {
        // Requires type & buffer size
        if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
        // Check if buffer type matches template
        if (Config::typeId<CScalar>() != src_cfg.type()) {
            ConfigError err;
            err << "Can not configure USBDemod: Invalid type " << src_cfg.type()
                << ", expected " << Config::typeId<CScalar>();
            throw err;
        }

        // Unreference previous buffer
        _buffer.unref();
        // Allocate buffer
        _buffer =  Buffer<Scalar>(src_cfg.bufferSize());

        LogMessage msg(LOG_DEBUG);
        msg << "Configure USBDemod: " << this << std::endl
            << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
            << " output type: " << Traits<Scalar>::scalarId << std::endl
            << " sample rate: " << src_cfg.sampleRate() << std::endl
            << " buffer size: " << src_cfg.bufferSize();
        Logger::get().log(msg);

        // Propergate config
        this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                               src_cfg.bufferSize(), 1));
    }

    virtual void process(const Buffer<CScalar> &buffer, bool allow_overwrite) {
        if (allow_overwrite) {
            // Process in-place
            _process(buffer, Buffer<Scalar>(buffer));
        } else {
            // Store result in buffer
            _process(buffer, _buffer);
        }
    }

protected:
    void _process(const Buffer< std::complex<Scalar> > &in, const Buffer< Scalar> &out) {
        for (size_t i=0; i<in.size(); i++) {
            out[i] = (SScalar(std::real(in[i])) + SScalar(std::imag(in[i])))/2;
        }
        this->send(out.head(in.size()));
    }

protected:
    Buffer<Scalar> _buffer;
};



template <class iScalar, class oScalar=iScalar>
class FMDemod: public Sink< std::complex<iScalar> >, public Source
{
public:
    typedef typename Traits<iScalar>::SScalar SScalar;

public:
    FMDemod():
        Sink< std::complex<iScalar> >(), Source(), _shift(0), _can_overwrite(false)
    {
        _shift = 8*(sizeof(oScalar)-sizeof(iScalar));
    }

    virtual ~FMDemod() {
        _buffer.unref();
    }

    virtual void config(const Config &src_cfg) {
        // Requires type & buffer size
        if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
        // Check if buffer type matches template
        if (Config::typeId< std::complex<iScalar> >() != src_cfg.type()) {
            ConfigError err;
            err << "Can not configure FMDemod: Invalid type " << src_cfg.type()
                << ", expected " << Config::typeId< std::complex<iScalar> >();
            throw err;
        }
        // Unreference buffer if non-empty
        if (! _buffer.isEmpty()) { _buffer.unref(); }
        // Allocate buffer
        _buffer =  Buffer<oScalar>(src_cfg.bufferSize());
        // reset last value
        _last_value = 0;
        // Check if FM demod can be performed in-place
        _can_overwrite = (sizeof(std::complex<iScalar>) >= sizeof(oScalar));

        LogMessage msg(LOG_DEBUG);
        msg << "Configured FMDemod node: " << this << std::endl
            << " sample-rate: " << src_cfg.sampleRate() << std::endl
            << " in-type / out-type: " << src_cfg.type()
            << " / " << Config::typeId<oScalar>() << std::endl
            << " in-place: " << (_can_overwrite ? "true" : "false") << std::endl
            << " output scale: 2^" << _shift;
        Logger::get().log(msg);

        // Propergate config
        this->setConfig(Config(Config::typeId<oScalar>(), src_cfg.sampleRate(),
                               src_cfg.bufferSize(), 1));
    }

    virtual void process(const Buffer<std::complex<iScalar> > &buffer, bool allow_overwrite)
    {
        if (0 == buffer.size()) { return; }

        if (allow_overwrite && _can_overwrite) {
            _process(buffer, Buffer<oScalar>(buffer));
        } else {
            _process(buffer, _buffer);
        }
    }

protected:
    void _process(const Buffer< std::complex<iScalar> > &in, const Buffer<oScalar> &out)
    {       
        // The last input value
        std::complex<iScalar> last_value = _last_value;
        // calc first value
        SScalar a = (SScalar(in[0].real())*SScalar(last_value.real()))/2
                + (SScalar(in[0].imag())*SScalar(last_value.imag()))/2;
        SScalar b = (SScalar(in[0].imag())*SScalar(last_value.real()))/2
                - (SScalar(in[0].real())*SScalar(last_value.imag()))/2;
        a >>= Traits<iScalar>::shift; b >>= Traits<iScalar>::shift;
        // update last value
        last_value = in[0];
        // calc output (prob. overwriting the last value)
        out[0] = _gain_const*fast_atan2<iScalar, oScalar>(a, b);
        //out[0] = _gain_const*(1<<12)*(std::atan2(float(a),float(b))/M_PI);

        // Calc remaining values
        for (size_t i=1; i<in.size(); i++) {
            a = (SScalar(in[i].real())*SScalar(last_value.real()))/2
                    + (SScalar(in[i].imag())*SScalar(last_value.imag()))/2;
            b = (SScalar(in[i].imag())*SScalar(last_value.real()))/2
                    - (SScalar(in[i].real())*SScalar(last_value.imag()))/2;
            a >>= Traits<iScalar>::shift; b >>= Traits<iScalar>::shift;
            last_value = in[i];
            out[i] = _gain_const*fast_atan2<iScalar,oScalar>(a, b);
            //out[i] = _gain_const*(1<<12)*(std::atan2(float(a),float(b))/M_PI);
        }

        // Store last value
        _last_value = last_value;
        // propergate result
        this->send(out.head(in.size()));
    }


protected:
    int _shift;
    std::complex<iScalar> _last_value;
    bool _can_overwrite;
    Buffer<oScalar> _buffer;
    float _gain_const = 2.0;
};


template <class Scalar>
class FMDeemph: public Sink<Scalar>, public Source
{
public:
    FMDeemph(bool enabled=true)
        : Sink<Scalar>(), Source(), _enabled(enabled), _alpha(0), _avg(0), _buffer(0)
    {
        // pass...
    }

    virtual ~FMDeemph() {
        _buffer.unref();
    }

    inline bool isEnabled() const { return _enabled; }

    inline void enable(bool enabled) { _enabled = enabled; }

    virtual void config(const Config &src_cfg) {
        // Requires type, sample rate & buffer size
        if (!src_cfg.hasType() || !src_cfg.hasSampleRate() || !src_cfg.hasBufferSize()) { return; }
        // Check if buffer type matches template
        if (Config::typeId<Scalar>() != src_cfg.type()) {
            ConfigError err;
            err << "Can not configure FMDeemph: Invalid type " << src_cfg.type()
                << ", expected " << Config::typeId<Scalar>();
            throw err;
        }
        // Determine filter constant alpha:
        _alpha = (int)round(
                    1.0/( (1.0-exp(-1.0/(src_cfg.sampleRate() * 75e-6) )) ) );
        // Reset average:
        _avg = 0;
        // Unreference previous buffer
        _buffer.unref();
        // Allocate buffer:
        _buffer = Buffer<Scalar>(src_cfg.bufferSize());

        LogMessage msg(LOG_DEBUG);
        msg << "Configured FMDDeemph node: " << this << std::endl
            << " sample-rate: " << src_cfg.sampleRate() << std::endl
            << " type: " << src_cfg.type();
        Logger::get().log(msg);

        // Propergate config:
        this->setConfig(Config(src_cfg.type(), src_cfg.sampleRate(), src_cfg.bufferSize(), 1));
    }

    virtual void process(const Buffer<Scalar> &buffer, bool allow_overwrite)
    {        
        // Skip if disabled:
        if (!_enabled) { this->send(buffer, allow_overwrite); return; }

        // Process in-place or not
        if (allow_overwrite) {
            _process(buffer, buffer);
            this->send(buffer, allow_overwrite);
        } else {
            _process(buffer, _buffer);
            this->send(_buffer.head(buffer.size()), false);
        }
    }

protected:
    void _process(const Buffer<Scalar> &in, const Buffer<Scalar> &out) {
        for (size_t i=0; i<in.size(); i++) {
            // Update average:
            Scalar diff = in[i] - _avg;
            if (diff > 0) { _avg += (diff + _alpha/2) / _alpha; }
            else { _avg += (diff - _alpha/2) / _alpha; }
            // Store result
            out[i] = _avg;
        }
    }

protected:
    bool _enabled;
    int _alpha;
    Scalar _avg;
    Buffer<Scalar> _buffer;
};

}

#endif // __SDR_DEMOD_HH__
