#include <QtQml/qqmlprivate.h>
#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

namespace QmlCacheGeneratedCode {
namespace _0x5f_SqlSyncAdmin_qml_main_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_WizardSidebar_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_pages_ConnectionPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_pages_ConfigurePage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_pages_InstallPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_pages_SummaryPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SqlSyncAdmin_qml_pages_UsersPage_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}

}
namespace {
struct Registry {
    Registry();
    ~Registry();
    QHash<QString, const QQmlPrivate::CachedQmlUnit*> resourcePathToCachedUnit;
    static const QQmlPrivate::CachedQmlUnit *lookupCachedUnit(const QUrl &url);
};

Q_GLOBAL_STATIC(Registry, unitRegistry)


Registry::Registry() {
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/main.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_main_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/WizardSidebar.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_WizardSidebar_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/pages/ConnectionPage.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_pages_ConnectionPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/pages/ConfigurePage.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_pages_ConfigurePage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/pages/InstallPage.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_pages_InstallPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/pages/SummaryPage.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_pages_SummaryPage_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SqlSyncAdmin/qml/pages/UsersPage.qml"), &QmlCacheGeneratedCode::_0x5f_SqlSyncAdmin_qml_pages_UsersPage_qml::unit);
    QQmlPrivate::RegisterQmlUnitCacheHook registration;
    registration.structVersion = 0;
    registration.lookupCachedQmlUnit = &lookupCachedUnit;
    QQmlPrivate::qmlregister(QQmlPrivate::QmlUnitCacheHookRegistration, &registration);
}

Registry::~Registry() {
    QQmlPrivate::qmlunregister(QQmlPrivate::QmlUnitCacheHookRegistration, quintptr(&lookupCachedUnit));
}

const QQmlPrivate::CachedQmlUnit *Registry::lookupCachedUnit(const QUrl &url) {
    if (url.scheme() != QLatin1String("qrc"))
        return nullptr;
    QString resourcePath = QDir::cleanPath(url.path());
    if (resourcePath.isEmpty())
        return nullptr;
    if (!resourcePath.startsWith(QLatin1Char('/')))
        resourcePath.prepend(QLatin1Char('/'));
    return unitRegistry()->resourcePathToCachedUnit.value(resourcePath, nullptr);
}
}
int QT_MANGLE_NAMESPACE(qInitResources_qmlcache_SQLSyncAdmin)() {
    ::unitRegistry();
    return 1;
}
Q_CONSTRUCTOR_FUNCTION(QT_MANGLE_NAMESPACE(qInitResources_qmlcache_SQLSyncAdmin))
int QT_MANGLE_NAMESPACE(qCleanupResources_qmlcache_SQLSyncAdmin)() {
    return 1;
}
