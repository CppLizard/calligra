/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KIS_SIGNAL_COMPRESSOR_WITH_PARAM_H
#define __KIS_SIGNAL_COMPRESSOR_WITH_PARAM_H

#include <kis_signal_compressor.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

/**
 * A special class that converts a Qt signal into a boost::function
 * call.
 *
 * Example:
 *
 * boost::function<void ()> destinationFunctionCall(boost::bind(someNiceFunc, firstParam, secondParam));
 * SignalToFunctionProxy proxy(destinationFunctionCall);
 * connect(srcObject, SIGNAL(sigSomethingChanged()), &proxy, SLOT(start()));
 *
 * Now every time sigSomethingChanged() is emitted, someNiceFunc is
 * called. boost::bind allows us to call any method of any class without
 * changing signature of the class or creating special wrappers.
 */
class KRITAIMAGE_EXPORT SignalToFunctionProxy : public QObject
{
    Q_OBJECT
public:
    typedef boost::function<void ()> TrivialFunction;

public:
    SignalToFunctionProxy(TrivialFunction function)
        : m_function(function)
    {
    }

public Q_SLOTS:
    void start() {
        m_function();
    }

private:
    TrivialFunction m_function;
};


/**
 * A special class for deferring and comressing events with one
 * parameter of type T. This works like KisSignalCompressor but can
 * handle events with one parameter. Due to limitation of the Qt this
 * doesn't allow signal/slots, so it uses boost::funxtion instead.
 *
 * In the end (after a timeout) the latest param value is returned to
 * the callback.
 *
 *        Usage:
 *
 *        \code{.cpp}
 *
 *        // prepare the callback function
 *        boost::function<void (qreal)> callback(
 *            boost::bind(&LutDockerDock::setCurrentExposureImpl, this, _1));
 *
 *        // Create the compressor object
 *        KisSignalCompressorWithParam<qreal> compressor(40, callback);
 *
 *        // When event comes:
 *        compressor.start(0.123456);
 *
 *        \endcode
 */

template <typename T>
class KisSignalCompressorWithParam
{
public:
    typedef boost::function<void (T)> CallbackFunction;

public:
    KisSignalCompressorWithParam(int delay, CallbackFunction function)
        : m_compressor(delay, KisSignalCompressor::FIRST_ACTIVE),
          m_function(function)
    {
        boost::function<void ()> callback(
            boost::bind(&KisSignalCompressorWithParam<T>::fakeSlotTimeout, this));
        m_signalProxy.reset(new SignalToFunctionProxy(callback));

        m_compressor.connect(&m_compressor, SIGNAL(timeout()), m_signalProxy.data(), SLOT(start()));
    }

    ~KisSignalCompressorWithParam()
    {
    }

    void start(T param) {
        m_currentParamValue = param;
        m_compressor.start();
    }

private:
    void fakeSlotTimeout() {
        m_function(m_currentParamValue);
    }

private:
    KisSignalCompressor m_compressor;
    CallbackFunction m_function;
    QScopedPointer<SignalToFunctionProxy> m_signalProxy;
    T m_currentParamValue;
};

#endif /* __KIS_SIGNAL_COMPRESSOR_WITH_PARAM_H */
