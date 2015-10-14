/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmodbusserver.h"
#include "qmodbusserver_p.h"

#include <bitset>

QT_BEGIN_NAMESPACE

/*!
    \class QModbusServer
    \inmodule QtSerialBus
    \since 5.6

    \brief The QModbusServer class is the interface to receive and process Modbus requests.

    Modbus networks can have multiple Modbus servers. Modbus Servers are read/written by a
    Modbus client represented by \l QModbusClient. QModbusServer communicates with a Modbus
    backend, providing users with a convenient API.

 */

/*!
    Constructs a Modbus server with the specified \a parent.
 */
QModbusServer::QModbusServer(QObject *parent) :
    QModbusDevice(*new QModbusServerPrivate, parent)
{
}

/*!
    \internal
*/
QModbusServer::~QModbusServer()
{
}

/*!
    \internal
 */
QModbusServer::QModbusServer(QModbusServerPrivate &dd, QObject *parent) :
    QModbusDevice(dd, parent)
{
}

/*!
    Sets the registered map structure for requests from other ModBus clients to \a map.
    The register values are initialized with zero. Returns \c true on success; otherwise \c false.

    If this function is not called before connecting, a default register with zero
    entries is setup.

    \note Calling this function discards any register value that was previously set.
 */
bool QModbusServer::setMap(const QModbusDataUnitMap &map)
{
    return d_func()->setMap(map);
}


/*!
    Reads data stored in the Modbus server. A Modbus server has four tables (\a table) and each
    have a unique \a address field, which is used to read \a data from the desired field.
    See QModbusDataUnit::RegisterType for more information about the different tables.
    Returns \c false if address is outside of the map range or the register type is not even defined.

    \sa QModbusDataUnit::RegisterType, setData()
 */
bool QModbusServer::data(QModbusDataUnit::RegisterType table,
                         quint16 address, quint16 *data) const
{
    Q_D(const QModbusServer);
    if (!data)
        return false;

    const QModbusDataUnit *unit;

    switch (table) {
    case QModbusDataUnit::Invalid:
        return false;
    case QModbusDataUnit::DiscreteInputs:
        unit = &(d->m_discreteInputs);
        break;
    case QModbusDataUnit::Coils:
        unit = &(d->m_coils);
        break;
    case QModbusDataUnit::InputRegisters:
        unit = &(d->m_inputRegisters);
        break;
    case QModbusDataUnit::HoldingRegisters:
        unit = &(d->m_holdingRegisters);
        break;
    }

    if (!unit->isValid()
        || address < unit->startAddress()
        || address >= (unit->startAddress() + unit->valueCount())) {
        return false;
    }

    *data = unit->value(address);

    return true;
}

/*!
    Returns the values in the register range given by \a results.

    \a newData must provide a valid register type, start address
    and valueCount. The returned \a results will contain the register values
    associated with the given range.

    If \a newData contains a valid register type but a negative start address
    the entire register map is returned and \a newData appropriately sized.
 */
bool QModbusServer::data(QModbusDataUnit *newData) const
{
    Q_D(const QModbusServer);

    if (!newData)
        return false;

    const QModbusDataUnit *current;

    switch (newData->registerType()) {
    case QModbusDataUnit::Invalid:
        return false;
    case QModbusDataUnit::DiscreteInputs:
        current = &(d->m_discreteInputs);
        break;
    case QModbusDataUnit::Coils:
        current = &(d->m_coils);
        break;
    case QModbusDataUnit::InputRegisters:
        current = &(d->m_inputRegisters);
        break;
    case QModbusDataUnit::HoldingRegisters:
        current = &(d->m_holdingRegisters);
        break;
    }

    if (newData->startAddress() < 0) { //return enire map for given type
        *newData = *current;
        return true;
    }

    //check range start is within internal map range
    int internalRangeEndAddress = current->startAddress() + current->valueCount() - 1;
    if (newData->startAddress() < current->startAddress()
        || newData->startAddress() > internalRangeEndAddress) {
        return false;
    }

    //check range end is within internal map range
    int rangeEndAddress = newData->startAddress() + newData->valueCount() - 1;
    if (rangeEndAddress < current->startAddress()
        || rangeEndAddress > internalRangeEndAddress) {
        return false;
    }

    newData->setValues(current->values().mid(newData->startAddress(),
                                             newData->valueCount()));
    return true;
}

/*!
    Writes data to the Modbus server. A Modbus server has four tables (\a table) and each have a
    unique \a address field, which is used to write \a data to the desired field.
    Returns \c false if address outside of the map range.

    \sa QModbusDataUnit::RegisterType, data()
 */

bool QModbusServer::setData(QModbusDataUnit::RegisterType table,
                            quint16 address, quint16 data)
{
    Q_D(QModbusServer);
    QModbusDataUnit* unit;

    switch (table) {
    case QModbusDataUnit::Invalid:
        return false;
    case QModbusDataUnit::DiscreteInputs:
        unit = &(d->m_discreteInputs);
        break;
    case QModbusDataUnit::Coils:
        unit = &(d->m_coils);
        break;
    case QModbusDataUnit::InputRegisters:
        unit = &(d->m_inputRegisters);
        break;
    case QModbusDataUnit::HoldingRegisters:
        unit = &(d->m_holdingRegisters);
        break;
    }

    if (!unit->isValid()
        || address < unit->startAddress()
        || address >= (unit->startAddress() + unit->valueCount())) {
        return false;
    }

    unit->setValue(address, data);
    return true;
}

/*!
    Writes \a newData to the Modbus server map.
    Returns \c false if the \a newData range is outside of the map range.

    \sa data()
 */

bool QModbusServer::setData(const QModbusDataUnit &newData)
{
    Q_D(QModbusServer);

    QModbusDataUnit *current = Q_NULLPTR;

    switch (newData.registerType()) {
    case QModbusDataUnit::Invalid:
        return false;
    case QModbusDataUnit::DiscreteInputs:
        current = &(d->m_discreteInputs);
        break;
    case QModbusDataUnit::Coils:
        current = &(d->m_coils);
        break;
    case QModbusDataUnit::InputRegisters:
        current = &(d->m_inputRegisters);
        break;
    case QModbusDataUnit::HoldingRegisters:
        current = &(d->m_holdingRegisters);
        break;
    }

    if (!current->isValid())
        return false;

    //check range start is within internal map range
    int internalRangeEndAddress = current->startAddress() + current->valueCount() - 1;
    if (newData.startAddress() < current->startAddress()
        || newData.startAddress() > internalRangeEndAddress) {
        return false;
    }

    //check range end is within internal map range
    int rangeEndAddress = newData.startAddress() + newData.valueCount() - 1;
    if (rangeEndAddress < current->startAddress()
        || rangeEndAddress > internalRangeEndAddress) {
        return false;
    }

    for (int i = newData.startAddress();
         i < newData.startAddress() + int(newData.valueCount()); i++)
        current->setValue(i, newData.value(i-newData.startAddress()));

    return true;
}

/*!
    \fn int QModbusServer::slaveId() const
    Multiple Modbus devices can be connected together on the same physical link.
    Slave id is a unique identifier that each Modbus server must have, and it is used
    to filter out incoming messages.

    Returns slave id.

    \sa setSlaveId()
 */

/*!
    \fn void QModbusServer::setSlaveId(int id)
    Multiple Modbus devices can be connected together on the same physical link.
    So it is important that each server is identified by a unique id.

    Sets \a id as slave id.

    \sa slaveId()
 */

/*!
    \fn void QModbusServer::dataWritten(QModbusDataUnit::RegisterType table, int address, int size)

    This signal is emitted when a Modbus client has written one or more fields of data to the
    Modbus server. Signal contains information about the fields that were written:
    \list
     \li \a table that was written,
     \li \a address of the first field that was written,
     \li and \a size of the consecutive fields that were written starting from \a address.
    \endlist
 */

/*
    Processes a Modbus client \a request and returns a Modbus response.
*/
QModbusResponse QModbusServer::processRequest(const QModbusPdu &request)
{
    return d_func()->processRequest(request);
}

/*
    To be implemented by custom Modbus server implementation. The default implementation returns
    a \c QModbusExceptionResponse with the \a request function code and error code set to illegal
    function.
*/
QModbusResponse QModbusServer::processCustomRequest(const QModbusPdu &request)
{
    return QModbusExceptionResponse(request.functionCode(),
        QModbusExceptionResponse::IllegalFunction);
}


// -- QModbusServerPrivate

bool QModbusServerPrivate::setMap(const QModbusDataUnitMap &map)
{
    m_discreteInputs = map.value(QModbusDataUnit::DiscreteInputs);
    m_coils = map.value(QModbusDataUnit::Coils);
    m_inputRegisters = map.value(QModbusDataUnit::InputRegisters);
    m_holdingRegisters = map.value(QModbusDataUnit::HoldingRegisters);
    return true;
}

/*
    TODO: implement
*/
QModbusResponse QModbusServerPrivate::processRequest(const QModbusPdu &request)
{
    switch (request.functionCode()) {
    case QModbusRequest::ReadCoils:
        return processReadCoilsRequest(request);
    case QModbusRequest::ReadDiscreteInputs:
    case QModbusRequest::ReadHoldingRegisters:
    case QModbusRequest::ReadInputRegisters:
    case QModbusRequest::WriteSingleCoil:
        return processWriteSingleCoilRequest(request);
    case QModbusRequest::WriteSingleRegister:
    case QModbusRequest::ReadExceptionStatus:
    case QModbusRequest::Diagnostics:
    case QModbusRequest::GetCommEventCounter:
    case QModbusRequest::GetCommEventLog:
    case QModbusRequest::WriteMultipleCoils:
        return processWriteMultipleCoilsRequest(request);
    case QModbusRequest::WriteMultipleRegisters:
    case QModbusRequest::ReportServerId:
    case QModbusRequest::ReadFileRecord:
    case QModbusRequest::WriteFileRecord:
    case QModbusRequest::MaskWriteRegister:
    case QModbusRequest::ReadWriteMultipleRegisters:
    case QModbusRequest::ReadFifoQueue:
    case QModbusRequest::EncapsulatedInterfaceTransport:
    default:
        break;
    }
    return q_func()->processCustomRequest(request);
}

QModbusResponse QModbusServerPrivate::processReadCoilsRequest(const QModbusRequest &request)
{
    quint16 address, numberOfCoils;
    request.decodeData(&address, &numberOfCoils);

    if ((numberOfCoils < 0x0001) || (numberOfCoils > 0x07D0)) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataValue);
    }

    if ((m_coils.startAddress() > address)
        || ((m_coils.startAddress() + m_coils.valueCount()) < (address + numberOfCoils))) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataAddress);
    }

    // Get the requested range out of the registers.
    QVector<quint16> data = m_coils.values().mid(address, numberOfCoils);
    quint8 byteCount = numberOfCoils / 8;
    if ((data.count() % 8) != 0) {
        byteCount += 1;
        // If the range is not a multiple of 8, resize.
        data.resize(byteCount * 8);
        // According to the spec: If the returned output quantity is not a multiple of
        // eight, the remaining bits in the final data byte will be padded with zeros.
        data.insert(numberOfCoils, (byteCount * 8) - numberOfCoils, 0u);
    }

    address = 0; // The data range now starts with zero.
    QVector<quint8> bytes;
    for (int i = 0; i < byteCount; ++i) {
        std::bitset<8> byte;
        for (int currentBit = 0; currentBit < 8; ++currentBit)
            byte[currentBit] = data[address++];
        bytes.append(static_cast<quint8> (byte.to_ulong()));
    }

    // TODO: Increase message counters when they are implemented
    return QModbusResponse(request.functionCode(), byteCount, bytes);
}

QModbusResponse QModbusServerPrivate::processWriteSingleCoilRequest(const QModbusRequest &request)
{
    quint16 address, value;
    request.decodeData(&address, &value);

    if ((value != 0x0000) && (value != 0xFF00)) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataValue);
    }

    if ((m_coils.startAddress() > address)
        || ((m_coils.startAddress() + m_coils.valueCount()) < address)) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataAddress);
    }

    QVector<quint16> data = m_coils.values();
    data[address] = value;
    m_coils.setValues(data);

    // - TODO: Increase message counters when they are implemented
    return QModbusResponse(request.functionCode(), address, value);
}

QModbusResponse QModbusServerPrivate::processWriteMultipleCoilsRequest(const QModbusRequest &request)
{
    quint16 address, numberOfCoils;
    quint8 byteCount;
    request.decodeData(&address, &numberOfCoils, &byteCount);

    quint8 expectedBytes = numberOfCoils / 8;
    if ((numberOfCoils % 8) != 0)
        expectedBytes += 1;

    if ((numberOfCoils < 0x0001) || (numberOfCoils > 0x07B0) || (expectedBytes != byteCount)) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataValue);
    }

    if ((m_coils.startAddress() > address)
        || ((m_coils.startAddress() + m_coils.valueCount()) < (address + numberOfCoils))) {
        return QModbusExceptionResponse(request.functionCode(),
            QModbusExceptionResponse::IllegalDataAddress);
    }

    QVector<std::bitset<8>> bytes;
    const QByteArray payload = request.data().mid(5);
    for (qint32 i = payload.size() - 1; i >= 0; --i)
        bytes.append(quint8(payload[i]));

    QVector<quint16> values = m_coils.values();

    quint16 coil = address + numberOfCoils;
    qint32 currentBit = 8 - ((byteCount * 8) - numberOfCoils);
    foreach (const auto &currentByte, bytes) {
        for (currentBit -= 1; currentBit >= 0; --currentBit)
            values[--coil] = currentByte[currentBit];
        currentBit = 8;
    }

    m_coils.setValues(values);

    // - TODO: Increase message counters when they are implemented
    return QModbusResponse(request.functionCode(), address, numberOfCoils);
}

QT_END_NAMESPACE