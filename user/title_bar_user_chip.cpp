#include "title_bar_user_chip.h"
#include "user_session.h"
#include <SARibbonBar/SARibbonToolButton.h>

#include <QHBoxLayout>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QSslSocket>

QJ_NAMESPACE_FIT_USER_BEGIN

namespace
{
constexpr char kNeutralAvatarRes[] = ":/ultramill/resource/avatar.png";

QString AvatarRibbonToolButtonStyleSheet()
{
    return QStringLiteral(
        "SARibbonToolButton#CamTitleBarUserAvatar,"
        "SARibbonToolButton#CamTitleBarUserAvatar:hover,"
        "SARibbonToolButton#CamTitleBarUserAvatar:pressed {"
        "  background-color: transparent;"
        "  border: none;"
        "  border-radius: 999px;"
        "  padding: 0;""}");
}
}

TitleBarUserChip::TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl)
    : QWidget(parent)
    , _apiBaseUrl(apiBaseUrl)
    , _nam(new QNetworkAccessManager(this))
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(TitleBarUserChip::kAvatarButtonSide);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(QStringLiteral("TitleBarUserChip { background: transparent; }"));

    _avatarButton = new SARibbonToolButton(this);
    _avatarButton->setObjectName(QStringLiteral("CamTitleBarUserAvatar"));
    _avatarButton->setAttribute(Qt::WA_StyledBackground, true);
    _avatarButton->setStyleSheet(AvatarRibbonToolButtonStyleSheet());
    _avatarButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _avatarButton->setSmallIconSize(QSize(TitleBarUserChip::kAvatarIconSide, TitleBarUserChip::kAvatarIconSide));
    _avatarButton->setIconSize(QSize(TitleBarUserChip::kAvatarIconSide, TitleBarUserChip::kAvatarIconSide));
    _avatarButton->setFixedSize(QSize(TitleBarUserChip::kAvatarButtonSide, TitleBarUserChip::kAvatarButtonSide));
    _avatarButton->setFocusPolicy(Qt::NoFocus);
    _avatarButton->setCursor(Qt::PointingHandCursor);
    connect(_avatarButton, &QToolButton::clicked, this, [this]() {
        if (_loggedIn) {
            emit accountMenuRequested();
        } else {
            emit loginRequested();
        }
    });

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 4, 0);
    lay->setSpacing(0);
    lay->setAlignment(Qt::AlignVCenter);
    lay->addWidget(_avatarButton);

    connect(_nam, &QNetworkAccessManager::finished, this, &TitleBarUserChip::OnAvatarDownloadFinished);
}

void TitleBarUserChip::RelayoutInParent()
{
    updateGeometry();
    adjustSize();
    QWidget* w = parentWidget();
    for (int i = 0; w && i < 8; ++i, w = w->parentWidget()) {
        if (QLayout* lay = w->layout()) {
            lay->invalidate();
            lay->activate();
        }
        w->updateGeometry();
    }
}

void TitleBarUserChip::SyncFromSession(const UserSession* session)
{
    AbortAvatarRequest();
    _loggedIn = session && session->IsAuthenticated();
    if (!_loggedIn) {
        ApplyDefaultAvatar();
        return;
    }
    ApplyLoggedInAppearance(session);
}

void TitleBarUserChip::ApplyDefaultAvatar()
{
    _fallbackNickName.clear();
    _fallbackUserName.clear();
    const QPixmap ph = LoadAvatarRaster(kNeutralAvatarRes, TitleBarUserChip::kAvatarIconSide * 2);
    _avatarButton->setIcon(QIcon(MakeCircularAvatar(ph)));
    _avatarButton->setToolTip(tr("Not logged in"));
}

void TitleBarUserChip::ApplyLoggedInAppearance(const UserSession* session)
{
    const QVariantMap u = session->CurrentUser();
    const QString nick = u.value(QStringLiteral("nickName")).toString().trimmed();
    const QString userName = u.value(QStringLiteral("userName")).toString().trimmed();
    _fallbackNickName = nick;
    _fallbackUserName = userName;
    {
        const QString tip = !nick.isEmpty() ? nick : userName;
        _avatarButton->setToolTip(tip.trimmed().isEmpty() ? QString() : tip);
    }

    const QPixmap loggedInPlaceholder = MakeCircularAvatar(
        LoadAvatarRaster(kNeutralAvatarRes, TitleBarUserChip::kAvatarIconSide * 2));

    const QString raw = u.value(QStringLiteral("avatar")).toString().trimmed();
    if (raw.isEmpty()) {
        _avatarButton->setIcon(QIcon(MakeInitialAvatarWithRing(_fallbackNickName, _fallbackUserName)));
        return;
    }

    const QUrl url = ResolveAvatarUrl(raw);
    if (!url.isValid()) {
        _avatarButton->setIcon(QIcon(MakeInitialAvatarWithRing(_fallbackNickName, _fallbackUserName)));
        return;
    }

    _avatarButton->setIcon(QIcon(loggedInPlaceholder));
    _avatarReply = _nam->get(QNetworkRequest(url));
}

QUrl TitleBarUserChip::ResolveAvatarUrl(const QString& raw) const
{
    if (raw.isEmpty()) {
        return {};
    }
    QUrl u = QUrl::fromUserInput(raw);
    if (u.isRelative()) {
        return _apiBaseUrl.resolved(u);
    }

    if (u.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !QSslSocket::supportsSsl()) {
        u.setScheme(QStringLiteral("http"));
    }

    return u;
}

QString TitleBarUserChip::PickInitialChar(const QString& nickName, const QString& userName)
{
    const QString nick = nickName.trimmed();
    const QString user = userName.trimmed();
    QString seed = !nick.isEmpty() ? nick : user;
    if (seed.isEmpty()) {
        return QStringLiteral("?");
    }

    const QChar c = seed.at(0);
    if (c.isLetter() && c.unicode() <= 0x7F) {
        return QString(c.toUpper());
    }
    return QString(c);
}

QPixmap TitleBarUserChip::MakeInitialAvatarWithRing(const QString& nickName, const QString& userName) const
{
    const int side = TitleBarUserChip::kAvatarIconSide;
    QPixmap out(side, side);
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#999999")));
    painter.drawEllipse(0, 0, side, side);

    QFont f = font();
    f.setBold(true);
    f.setPixelSize(11);
    painter.setFont(f);
    painter.setPen(QColor(Qt::white));
    painter.drawText(QRect(0, 0, side, side), Qt::AlignCenter, PickInitialChar(nickName, userName));
    return out;
}

QPixmap TitleBarUserChip::LoadAvatarRaster(const char* resourcePath, int side)
{
    QPixmap loaded;
    if (!loaded.load(QString::fromUtf8(resourcePath))) {
        QPixmap px(side, side);
        px.fill(QColor(QStringLiteral("#E0E0E0")));
        return px;
    }
    return loaded.scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

QPixmap TitleBarUserChip::MakeCircularAvatar(const QPixmap& source) const
{
    const int side = TitleBarUserChip::kAvatarIconSide;
    QPixmap out(side, side);
    out.fill(Qt::transparent);
    if (source.isNull()) {
        return out;
    }
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    const QPixmap scaled = source.scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPainterPath clip;
    clip.addEllipse(0, 0, side, side);
    painter.setClipPath(clip);
    const int x = (side - scaled.width()) / 2;
    const int y = (side - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    return out;
}

void TitleBarUserChip::AbortAvatarRequest()
{
    if (_avatarReply) {
        disconnect(_avatarReply, nullptr, this, nullptr);
        _avatarReply->abort();
        _avatarReply->deleteLater();
        _avatarReply = nullptr;
    }
}

void TitleBarUserChip::OnAvatarDownloadFinished(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }
    if (reply != _avatarReply) {
        reply->deleteLater();
        return;
    }
    _avatarReply = nullptr;

    QPixmap loaded;
    if (reply->error() == QNetworkReply::NoError) {
        loaded.loadFromData(reply->readAll());
    }
    reply->deleteLater();

    if (loaded.isNull()) {
        _avatarButton->setIcon(QIcon(MakeInitialAvatarWithRing(_fallbackNickName, _fallbackUserName)));
    } else {
        _avatarButton->setIcon(QIcon(MakeCircularAvatar(loaded)));
    }
    RelayoutInParent();
    QTimer::singleShot(0, this, [this]() { RelayoutInParent(); });
}

QJ_NAMESPACE_FIT_USER_END


