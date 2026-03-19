/****************************************************************************
** Meta object code from reading C++ file 'AdminController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../admin/backend/AdminController.h"
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
        "serverModeChanged",
        "dbNameChanged",
        "supabaseUrlChanged",
        "supabaseServiceKeyChanged",
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
        "teardownStateChanged",
        "teardownStepCompleted",
        "teardownFinished",
        "userAdded",
        "userRemoved",
        "username",
        "userEdited",
        "testConnection",
        "checkIfAlreadySetup",
        "startSetup",
        "cancelSetup",
        "startTeardown",
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
        "serverMode",
        "dbName",
        "supabaseUrl",
        "supabaseServiceKey",
        "authenticatorPassword",
        "jwtSecret",
        "setupRunning",
        "setupDone",
        "totalSteps",
        "teardownRunning",
        "teardownDone",
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
        // Signal 'serverModeChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dbNameChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'supabaseUrlChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'supabaseServiceKeyChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'secretsReady'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'setupStateChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'usersChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectionTestResult'
        QtMocHelpers::SignalData<void(bool, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'stepCompleted'
        QtMocHelpers::SignalData<void(int, bool, const QString &, const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 17 }, { QMetaType::Bool, 14 }, { QMetaType::QString, 18 }, { QMetaType::QString, 19 },
        }}),
        // Signal 'setupFinished'
        QtMocHelpers::SignalData<void(bool, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'alreadySetup'
        QtMocHelpers::SignalData<void(bool)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 22 },
        }}),
        // Signal 'teardownStateChanged'
        QtMocHelpers::SignalData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'teardownStepCompleted'
        QtMocHelpers::SignalData<void(int, bool, const QString &, const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 17 }, { QMetaType::Bool, 14 }, { QMetaType::QString, 18 }, { QMetaType::QString, 19 },
        }}),
        // Signal 'teardownFinished'
        QtMocHelpers::SignalData<void(bool, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'userAdded'
        QtMocHelpers::SignalData<void(bool, const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'userRemoved'
        QtMocHelpers::SignalData<void(bool, const QString &)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 28 },
        }}),
        // Signal 'userEdited'
        QtMocHelpers::SignalData<void(bool, const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 14 }, { QMetaType::QString, 15 },
        }}),
        // Method 'testConnection'
        QtMocHelpers::MethodData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkIfAlreadySetup'
        QtMocHelpers::MethodData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'startSetup'
        QtMocHelpers::MethodData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'cancelSetup'
        QtMocHelpers::MethodData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'startTeardown'
        QtMocHelpers::MethodData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'saveConnectionSettings'
        QtMocHelpers::MethodData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'loadUsers'
        QtMocHelpers::MethodData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'addUser'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 28 }, { QMetaType::QString, 38 },
        }}),
        // Method 'removeUser'
        QtMocHelpers::MethodData<void(const QString &)>(39, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 28 },
        }}),
        // Method 'editUser'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 28 }, { QMetaType::QString, 41 },
        }}),
        // Method 'copyToClipboard'
        QtMocHelpers::MethodData<void(const QString &)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 43 },
        }}),
        // Method 'postgrestConfig'
        QtMocHelpers::MethodData<QString() const>(44, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'host'
        QtMocHelpers::PropertyData<QString>(45, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'port'
        QtMocHelpers::PropertyData<int>(46, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'superuser'
        QtMocHelpers::PropertyData<QString>(47, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'superPass'
        QtMocHelpers::PropertyData<QString>(48, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'serverMode'
        QtMocHelpers::PropertyData<int>(49, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'dbName'
        QtMocHelpers::PropertyData<QString>(50, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'supabaseUrl'
        QtMocHelpers::PropertyData<QString>(51, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'supabaseServiceKey'
        QtMocHelpers::PropertyData<QString>(52, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
        // property 'authenticatorPassword'
        QtMocHelpers::PropertyData<QString>(53, QMetaType::QString, QMC::DefaultPropertyFlags, 8),
        // property 'jwtSecret'
        QtMocHelpers::PropertyData<QString>(54, QMetaType::QString, QMC::DefaultPropertyFlags, 8),
        // property 'setupRunning'
        QtMocHelpers::PropertyData<bool>(55, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'setupDone'
        QtMocHelpers::PropertyData<bool>(56, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'totalSteps'
        QtMocHelpers::PropertyData<int>(57, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'teardownRunning'
        QtMocHelpers::PropertyData<bool>(58, QMetaType::Bool, QMC::DefaultPropertyFlags, 15),
        // property 'teardownDone'
        QtMocHelpers::PropertyData<bool>(59, QMetaType::Bool, QMC::DefaultPropertyFlags, 15),
        // property 'users'
        QtMocHelpers::PropertyData<QVariantList>(60, 0x80000000 | 61, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 10),
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
        case 4: _t->serverModeChanged(); break;
        case 5: _t->dbNameChanged(); break;
        case 6: _t->supabaseUrlChanged(); break;
        case 7: _t->supabaseServiceKeyChanged(); break;
        case 8: _t->secretsReady(); break;
        case 9: _t->setupStateChanged(); break;
        case 10: _t->usersChanged(); break;
        case 11: _t->connectionTestResult((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->stepCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 13: _t->setupFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->alreadySetup((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->teardownStateChanged(); break;
        case 16: _t->teardownStepCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 17: _t->teardownFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 18: _t->userAdded((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->userRemoved((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 20: _t->userEdited((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->testConnection(); break;
        case 22: _t->checkIfAlreadySetup(); break;
        case 23: _t->startSetup(); break;
        case 24: _t->cancelSetup(); break;
        case 25: _t->startTeardown(); break;
        case 26: _t->saveConnectionSettings(); break;
        case 27: _t->loadUsers(); break;
        case 28: _t->addUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 29: _t->removeUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 30: _t->editUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 31: _t->copyToClipboard((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 32: { QString _r = _t->postgrestConfig();
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
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::serverModeChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::dbNameChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::supabaseUrlChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::supabaseServiceKeyChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::secretsReady, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::setupStateChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::usersChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::connectionTestResult, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(int , bool , const QString & , const QString & )>(_a, &AdminController::stepCompleted, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::setupFinished, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool )>(_a, &AdminController::alreadySetup, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)()>(_a, &AdminController::teardownStateChanged, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(int , bool , const QString & , const QString & )>(_a, &AdminController::teardownStepCompleted, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::teardownFinished, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userAdded, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userRemoved, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdminController::*)(bool , const QString & )>(_a, &AdminController::userEdited, 20))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->host(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->port(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->superuser(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->superPass(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->serverMode(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->dbName(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->supabaseUrl(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->supabaseServiceKey(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->authenticatorPassword(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->jwtSecret(); break;
        case 10: *reinterpret_cast<bool*>(_v) = _t->isSetupRunning(); break;
        case 11: *reinterpret_cast<bool*>(_v) = _t->isSetupDone(); break;
        case 12: *reinterpret_cast<int*>(_v) = _t->totalSteps(); break;
        case 13: *reinterpret_cast<bool*>(_v) = _t->isTeardownRunning(); break;
        case 14: *reinterpret_cast<bool*>(_v) = _t->isTeardownDone(); break;
        case 15: *reinterpret_cast<QVariantList*>(_v) = _t->users(); break;
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
        case 4: _t->setServerMode(*reinterpret_cast<int*>(_v)); break;
        case 5: _t->setDbName(*reinterpret_cast<QString*>(_v)); break;
        case 6: _t->setSupabaseUrl(*reinterpret_cast<QString*>(_v)); break;
        case 7: _t->setSupabaseServiceKey(*reinterpret_cast<QString*>(_v)); break;
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
        if (_id < 33)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 33)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 33;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
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
void AdminController::serverModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AdminController::dbNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AdminController::supabaseUrlChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AdminController::supabaseServiceKeyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void AdminController::secretsReady()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void AdminController::setupStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void AdminController::usersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void AdminController::connectionTestResult(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2);
}

// SIGNAL 12
void AdminController::stepCompleted(int _t1, bool _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 13
void AdminController::setupFinished(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2);
}

// SIGNAL 14
void AdminController::alreadySetup(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1);
}

// SIGNAL 15
void AdminController::teardownStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void AdminController::teardownStepCompleted(int _t1, bool _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 17
void AdminController::teardownFinished(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 17, nullptr, _t1, _t2);
}

// SIGNAL 18
void AdminController::userAdded(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 18, nullptr, _t1, _t2);
}

// SIGNAL 19
void AdminController::userRemoved(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 19, nullptr, _t1, _t2);
}

// SIGNAL 20
void AdminController::userEdited(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 20, nullptr, _t1, _t2);
}
QT_WARNING_POP
