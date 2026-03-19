/****************************************************************************
** Meta object code from reading C++ file 'AdminController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../backend/AdminController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdminController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN15AdminControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto AdminController::qt_create_metaobjectdata<qt_meta_tag_ZN15AdminControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AdminController",
        "hostChanged",
        "",
        "portChanged",
        "superuserChanged",
        "superPassChanged",
        "secretsReady",
        "setupStateChanged",
        "usersChanged",
        "connectionTestResult",
        "success",
        "message",
        "stepCompleted",
        "index",
        "stepName",
        "detail",
        "setupFinished",
        "alreadySetup",
        "isSetup",
        "userAdded",
        "userRemoved",
        "username",
        "userEdited",
        "testConnection",
        "checkIfAlreadySetup",
        "startSetup",
        "cancelSetup",
        "saveConnectionSettings",
        "loadUsers",
        "addUser",
        "password",
        "removeUser",
        "editUser",
        "newPassword",
        "copyToClipboard",
        "text",
        "postgrestConfig",
        "host",
        "port",
        "superuser",
        "superPass",
        "authenticatorPassword",
        "jwtSecret",
        "setupRunning",
        "setupDone",
        "totalSteps",
        "users",
        "QVariantList"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'hostChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'portChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'superuserChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'superPassChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'secretsReady'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'setupStateChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'usersChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectionTestResult'
        QtMocHelpers::SignalData<void(bool, const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'stepCompleted'
        QtMocHelpers::SignalData<void(int, bool, const QString &, const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Bool, 10 }, { QMetaType::QString, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'setupFinished'
        QtMocHelpers::SignalData<void(bool, const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'alreadySetup'
        QtMocHelpers::SignalData<void(bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 18 },
        }}),
        // Signal 'userAdded'
        QtMocHelpers::SignalData<void(bool, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'userRemoved'
        QtMocHelpers::SignalData<void(bool, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 }, { QMetaType::QString, 21 },
        }}),
        // Signal 'userEdited'
        QtMocHelpers::SignalData<void(bool, const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 10 }, { QMetaType::QString, 11 },
        }}),
        // Method 'testConnection'
        QtMocHelpers::MethodData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkIfAlreadySetup'
        QtMocHelpers::MethodData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'startSetup'
        QtMocHelpers::MethodData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'cancelSetup'
        QtMocHelpers::MethodData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'saveConnectionSettings'
        QtMocHelpers::MethodData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'loadUsers'
        QtMocHelpers::MethodData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'addUser'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 30 },
        }}),
        // Method 'removeUser'
        QtMocHelpers::MethodData<void(const QString &)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'editUser'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 21 }, { QMetaType::QString, 33 },
        }}),
        // Method 'copyToClipboard'
        QtMocHelpers::MethodData<void(const QString &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 35 },
        }}),
        // Method 'postgrestConfig'
        QtMocHelpers::MethodData<QString() const>(36, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'host'
        QtMocHelpers::PropertyData<QString>(37, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'port'
        QtMocHelpers::PropertyData<int>(38, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'superuser'
        QtMocHelpers::PropertyData<QString>(39, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'superPass'
        QtMocHelpers::PropertyData<QString>(40, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'authenticatorPassword'
        QtMocHelpers::PropertyData<QString>(41, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'jwtSecret'
        QtMocHelpers::PropertyData<QString>(42, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'setupRunning'
        QtMocHelpers::PropertyData<bool>(43, QMetaType::Bool, QMC::DefaultPropertyFlags, 5),
        // property 'setupDone'
        QtMocHelpers::PropertyData<bool>(44, QMetaType::Bool, QMC::DefaultPropertyFlags, 5),
        // property 'totalSteps'
        QtMocHelpers::PropertyData<int>(45, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'users'
        QtMocHelpers::PropertyData<QVariantList>(46, 0x80000000 | 47, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 6),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AdminController, qt_meta_tag_ZN15AdminControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AdminController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AdminControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AdminControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15AdminControllerE_t>.metaTypes,
    nullptr
} };

void AdminController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AdminController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->hostChanged(); break;
        case 1: _t->portChanged(); break;
        case 2: _t->superuserChanged(); break;
        case 3: _t->superPassChanged(); break;
        case 4: _t->secretsReady(); break;
        case 5: _t->setupStateChanged(); break;
        case 6: _t->usersChanged(); break;
        case 7: _t->connectionTestResult((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->stepCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 9: _t->setupFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->alreadySetup((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->userAdded((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->userRemoved((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->userEdited((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->testConnection(); break;
        case 15: _t->checkIfAlreadySetup(); break;
        case 16: _t->startSetup(); break;
        case 17: _t->cancelSetup(); break;
        case 18: _t->saveConnectionSettings(); break;
        case 19: _t->loadUsers(); break;
        case 20: _t->addUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->removeUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->editUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 23: _t->copyToClipboard((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: { QString _r = _t->postgrestConfig();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::hostChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::portChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::superuserChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::superPassChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::secretsReady, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::setupStateChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::usersChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::connectionTestResult, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(int , bool , const QString & , const QString & )>(_a, &AdminController::stepCompleted, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::setupFinished, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool )>(_a, &AdminController::alreadySetup, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userAdded, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userRemoved, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userEdited, 13))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->host(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->port(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->superuser(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->superPass(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->authenticatorPassword(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->jwtSecret(); break;
        case 6: *reinterpret_cast<bool*>(_v) = _t->isSetupRunning(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->isSetupDone(); break;
        case 8: *reinterpret_cast<int*>(_v) = _t->totalSteps(); break;
        case 9: *reinterpret_cast<QVariantList*>(_v) = _t->users(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setHost(*reinterpret_cast<QString*>(_v)); break;
        case 1: _t->setPort(*reinterpret_cast<int*>(_v)); break;
        case 2: _t->setSuperuser(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setSuperPass(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *AdminController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdminController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AdminControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AdminController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 25;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void AdminController::hostChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AdminController::portChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AdminController::superuserChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AdminController::superPassChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AdminController::secretsReady()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AdminController::setupStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AdminController::usersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AdminController::connectionTestResult(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void AdminController::stepCompleted(int _t1, bool _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 9
void AdminController::setupFinished(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2);
}

// SIGNAL 10
void AdminController::alreadySetup(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void AdminController::userAdded(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2);
}

// SIGNAL 12
void AdminController::userRemoved(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2);
}

// SIGNAL 13
void AdminController::userEdited(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2);
}
QT_WARNING_POP
