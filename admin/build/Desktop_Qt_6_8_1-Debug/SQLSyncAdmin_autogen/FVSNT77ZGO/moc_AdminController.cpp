/****************************************************************************
** Meta object code from reading C++ file 'AdminController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../backend/AdminController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AdminController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.1. It"
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


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN15AdminControllerE = QtMocHelpers::stringData(
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
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN15AdminControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
      10,  231, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  164,    2, 0x06,   11 /* Public */,
       3,    0,  165,    2, 0x06,   12 /* Public */,
       4,    0,  166,    2, 0x06,   13 /* Public */,
       5,    0,  167,    2, 0x06,   14 /* Public */,
       6,    0,  168,    2, 0x06,   15 /* Public */,
       7,    0,  169,    2, 0x06,   16 /* Public */,
       8,    0,  170,    2, 0x06,   17 /* Public */,
       9,    2,  171,    2, 0x06,   18 /* Public */,
      12,    4,  176,    2, 0x06,   21 /* Public */,
      16,    2,  185,    2, 0x06,   26 /* Public */,
      17,    1,  190,    2, 0x06,   29 /* Public */,
      19,    2,  193,    2, 0x06,   31 /* Public */,
      20,    2,  198,    2, 0x06,   34 /* Public */,
      22,    2,  203,    2, 0x06,   37 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      23,    0,  208,    2, 0x02,   40 /* Public */,
      24,    0,  209,    2, 0x02,   41 /* Public */,
      25,    0,  210,    2, 0x02,   42 /* Public */,
      26,    0,  211,    2, 0x02,   43 /* Public */,
      27,    0,  212,    2, 0x02,   44 /* Public */,
      28,    0,  213,    2, 0x02,   45 /* Public */,
      29,    2,  214,    2, 0x02,   46 /* Public */,
      31,    1,  219,    2, 0x02,   49 /* Public */,
      32,    2,  222,    2, 0x02,   51 /* Public */,
      34,    1,  227,    2, 0x02,   54 /* Public */,
      36,    0,  230,    2, 0x102,   56 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool, QMetaType::QString, QMetaType::QString,   13,   10,   14,   15,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::Bool,   18,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   10,   21,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   10,   11,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   21,   30,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   21,   33,
    QMetaType::Void, QMetaType::QString,   35,
    QMetaType::QString,

 // properties: name, type, flags, notifyId, revision
      37, QMetaType::QString, 0x00015103, uint(0), 0,
      38, QMetaType::Int, 0x00015103, uint(1), 0,
      39, QMetaType::QString, 0x00015103, uint(2), 0,
      40, QMetaType::QString, 0x00015103, uint(3), 0,
      41, QMetaType::QString, 0x00015001, uint(4), 0,
      42, QMetaType::QString, 0x00015001, uint(4), 0,
      43, QMetaType::Bool, 0x00015001, uint(5), 0,
      44, QMetaType::Bool, 0x00015001, uint(5), 0,
      45, QMetaType::Int, 0x00015401, uint(-1), 0,
      46, 0x80000000 | 47, 0x00015009, uint(6), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject AdminController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN15AdminControllerE.offsetsAndSizes,
    qt_meta_data_ZN15AdminControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN15AdminControllerE_t,
        // property 'host'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'port'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'superuser'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'superPass'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'authenticatorPassword'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'jwtSecret'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'setupRunning'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'setupDone'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'totalSteps'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'users'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AdminController, std::true_type>,
        // method 'hostChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'portChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'superuserChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'superPassChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'secretsReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setupStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectionTestResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'stepCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setupFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'alreadySetup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'userAdded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'userRemoved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'userEdited'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'testConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkIfAlreadySetup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startSetup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelSetup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveConnectionSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadUsers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'editUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'copyToClipboard'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'postgrestConfig'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>
    >,
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
        case 7: _t->connectionTestResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->stepCompleted((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 9: _t->setupFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->alreadySetup((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->userAdded((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->userRemoved((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->userEdited((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->testConnection(); break;
        case 15: _t->checkIfAlreadySetup(); break;
        case 16: _t->startSetup(); break;
        case 17: _t->cancelSetup(); break;
        case 18: _t->saveConnectionSettings(); break;
        case 19: _t->loadUsers(); break;
        case 20: _t->addUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->removeUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->editUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 23: _t->copyToClipboard((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: { QString _r = _t->postgrestConfig();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::hostChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::portChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::superuserChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::superPassChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::secretsReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::setupStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)();
            if (_q_method_type _q_method = &AdminController::usersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool , const QString & );
            if (_q_method_type _q_method = &AdminController::connectionTestResult; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(int , bool , const QString & , const QString & );
            if (_q_method_type _q_method = &AdminController::stepCompleted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool , const QString & );
            if (_q_method_type _q_method = &AdminController::setupFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool );
            if (_q_method_type _q_method = &AdminController::alreadySetup; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool , const QString & );
            if (_q_method_type _q_method = &AdminController::userAdded; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool , const QString & );
            if (_q_method_type _q_method = &AdminController::userRemoved; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (AdminController::*)(bool , const QString & );
            if (_q_method_type _q_method = &AdminController::userEdited; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->host(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->port(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->superuser(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->superPass(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->authenticatorPassword(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->jwtSecret(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->isSetupRunning(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->isSetupDone(); break;
        case 8: *reinterpret_cast< int*>(_v) = _t->totalSteps(); break;
        case 9: *reinterpret_cast< QVariantList*>(_v) = _t->users(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setHost(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setPort(*reinterpret_cast< int*>(_v)); break;
        case 2: _t->setSuperuser(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setSuperPass(*reinterpret_cast< QString*>(_v)); break;
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
    if (!strcmp(_clname, qt_meta_stringdata_ZN15AdminControllerE.stringdata0))
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
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void AdminController::stepCompleted(int _t1, bool _t2, const QString & _t3, const QString & _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void AdminController::setupFinished(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void AdminController::alreadySetup(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void AdminController::userAdded(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void AdminController::userRemoved(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void AdminController::userEdited(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
QT_WARNING_POP
