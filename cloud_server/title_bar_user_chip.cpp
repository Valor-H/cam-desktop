#include "title_bar_user_chip.h"
#include "user_session.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointer>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

#include <hv/hssl.h>
#include <hv/requests.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

namespace
{
	constexpr char kNeutralAvatarRes[] = ":/ultramill/resource/avatar.png";
	constexpr int kAvatarDownloadTimeoutSec = 15;

	struct AvatarDownloadOutcome
	{
		bool networkOk{ false };
		int httpStatus{ 0 };
		QString requestUrl;
		QString errorMessage;
		QByteArray body;
	};

	QString BuildLibhvFailureMessage()
	{
		const char* backend = hssl_backend();
		const QString backendName = backend ? QString::fromUtf8(backend).trimmed() : QString();
		if (backendName.isEmpty()) {
			return QStringLiteral("libhv request failed");
		}
		return QStringLiteral("libhv request failed (ssl backend: %1)").arg(backendName);
	}

	QString AvatarRibbonToolButtonStyleSheet()
	{
		return QStringLiteral(
			"QToolButton#CamTitleBarUserAvatar,"
			"QToolButton#CamTitleBarUserAvatar:hover,"
			"QToolButton#CamTitleBarUserAvatar:pressed {"
			"  background-color: transparent;"
			"  border: none;"
			"  border-radius: 999px;"
			"  padding: 0;""}");
	}

	AvatarDownloadOutcome BuildAvatarDownloadOutcome(const QString& requestUrl, const HttpResponsePtr& resp)
	{
		AvatarDownloadOutcome outcome;
		outcome.networkOk = resp != nullptr;
		outcome.httpStatus = resp ? resp->status_code : 0;
		outcome.requestUrl = requestUrl;
		outcome.errorMessage = resp ? QString() : BuildLibhvFailureMessage();
		outcome.body = resp ? QByteArray::fromStdString(resp->body) : QByteArray();
		return outcome;
	}

}

TitleBarUserChip::TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl)
	: QWidget(parent)
	, _apiBaseUrl(apiBaseUrl)
	, _avatarRequestEpoch(std::make_shared<std::atomic<quint64>>(0))
{
	setCursor(Qt::PointingHandCursor);
	setMinimumHeight(TitleBarUserChip::kAvatarButtonSide);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setStyleSheet(QStringLiteral("TitleBarUserChip { background: transparent; }"));

	_avatarButton = new QToolButton(this);
	_avatarButton->setObjectName(QStringLiteral("CamTitleBarUserAvatar"));
	_avatarButton->setAttribute(Qt::WA_StyledBackground, true);
	_avatarButton->setStyleSheet(AvatarRibbonToolButtonStyleSheet());
	_avatarButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	_avatarButton->setIconSize(QSize(TitleBarUserChip::kAvatarIconSide, TitleBarUserChip::kAvatarIconSide));
	_avatarButton->setFixedSize(QSize(TitleBarUserChip::kAvatarButtonSide, TitleBarUserChip::kAvatarButtonSide));
	_avatarButton->setFocusPolicy(Qt::NoFocus);
	_avatarButton->setCursor(Qt::PointingHandCursor);
	connect(_avatarButton, &QToolButton::clicked, this, [this]() {
		if (_loggedIn) {
			emit accountMenuRequested();
		}
		else {
			emit loginRequested();
		}
		});

	auto* lay = new QHBoxLayout(this);
	lay->setContentsMargins(0, 0, 4, 0);
	lay->setSpacing(0);
	lay->setAlignment(Qt::AlignVCenter);
	lay->addWidget(_avatarButton);
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

void TitleBarUserChip::RefreshAvatarVisuals()
{
	if (!_avatarButton) {
		return;
	}

	_avatarButton->updateGeometry();
	_avatarButton->update();
	update();
	RelayoutInParent();

	QTimer::singleShot(0, this, [this]() {
		if (!_avatarButton) {
			return;
		}
		_avatarButton->update();
		update();
		RelayoutInParent();
		});
}

void TitleBarUserChip::ApplyAvatarIcon(const QPixmap& pixmap)
{
	_avatarButton->setIcon(QIcon(pixmap));
	RefreshAvatarVisuals();
}

void TitleBarUserChip::ApplyFallbackAvatar()
{
	if (_fallbackNickName.trimmed().isEmpty()) {
		ApplyPendingProfileAvatar();
		return;
	}
	ApplyAvatarIcon(MakeInitialAvatarWithRing(_fallbackNickName));
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
	const QPixmap ph = LoadAvatarRaster(kNeutralAvatarRes, TitleBarUserChip::kAvatarIconSide * 2);
	ApplyAvatarIcon(MakeCircularAvatar(ph));
	_avatarButton->setToolTip(tr("Not logged in"));
}

void TitleBarUserChip::ApplyPendingProfileAvatar()
{
	ApplyDefaultAvatar();
	_avatarButton->setToolTip(QString());
}

void TitleBarUserChip::ApplyLoggedInAppearance(const UserSession* session)
{
	const QVariantMap u = session->CurrentUser();
	if (u.isEmpty()) {
		ApplyPendingProfileAvatar();
		return;
	}

	const QString nick = u.value(QStringLiteral("nickName")).toString().trimmed();
	_fallbackNickName = nick;
	{
		const QString tip = !nick.isEmpty() ? nick : "None";
		_avatarButton->setToolTip(tip.trimmed().isEmpty() ? QString() : tip);
	}

	const QPixmap loggedInPlaceholder = MakeCircularAvatar(
		LoadAvatarRaster(kNeutralAvatarRes, TitleBarUserChip::kAvatarIconSide * 2));

	const QString raw = u.value(QStringLiteral("avatar")).toString().trimmed();
	if (raw.isEmpty()) {
		ApplyFallbackAvatar();
		return;
	}

	const QUrl url = ResolveAvatarUrl(raw);
	if (!url.isValid()) {
		ApplyFallbackAvatar();
		return;
	}

	ApplyAvatarIcon(loggedInPlaceholder);
	StartAvatarDownload(url);
}

QUrl TitleBarUserChip::ResolveAvatarUrl(const QString& raw) const
{
	if (raw.isEmpty()) {
		return {};
	}
	QUrl u = QUrl::fromEncoded(raw.toUtf8());
	if (!u.isValid()) {
		u = QUrl(raw, QUrl::StrictMode);
	}
	if (u.isRelative()) {
		return _apiBaseUrl.resolved(u);
	}
	return u;
}

QString TitleBarUserChip::PickInitialChar(const QString& nickName)
{
	const QString nick = nickName.trimmed();
	if (nick.isEmpty()) {
		return QString();
	}

	const QChar c = nick.at(0);
	if (c.isLetter() && c.unicode() <= 0x7F) {
		return QString(c.toUpper());
	}
	return QString(c);
}

QPixmap TitleBarUserChip::MakeInitialAvatarWithRing(const QString& nickName) const
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
	painter.drawText(QRect(0, 0, side, side), Qt::AlignCenter, PickInitialChar(nickName));
	return out;
}

QPixmap TitleBarUserChip::LoadAvatarRaster(const char* resourcePath, int side)
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(resourcePath), QSize(side, side));
	const QPixmap loaded = icon.pixmap(side, side);
	if (loaded.isNull()) {
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
	_avatarRequestEpoch->fetch_add(1, std::memory_order_relaxed);
}

void TitleBarUserChip::StartAvatarDownload(const QUrl& url)
{
	const quint64 requestId = _avatarRequestEpoch->load(std::memory_order_relaxed);
	auto epochRef = _avatarRequestEpoch;
	QPointer<TitleBarUserChip> self(this);

	const QString requestUrl = url.toString(QUrl::FullyEncoded);
	auto req = std::make_shared<HttpRequest>();
	req->method = HTTP_GET;
	req->url = requestUrl.toStdString();
	req->timeout = kAvatarDownloadTimeoutSec;

	requests::async(req, [epochRef, requestId, self, requestUrl](const HttpResponsePtr& resp) {
		QMetaObject::invokeMethod(
			QCoreApplication::instance(),
			[epochRef, requestId, self, outcome = BuildAvatarDownloadOutcome(requestUrl, resp)]() {
				if (!self) {
					return;
				}
				if (epochRef->load(std::memory_order_relaxed) != requestId) {
					return;
				}

				self->ApplyAvatarDownloadResult(outcome.networkOk,
					outcome.httpStatus,
					outcome.requestUrl,
					outcome.errorMessage,
					outcome.body);
			},
			Qt::QueuedConnection);
		});
}

void TitleBarUserChip::ApplyAvatarDownloadResult(bool networkOk,
	int httpStatus,
	const QString& requestUrl,
	const QString& errorMessage,
	const QByteArray& body)
{
	QPixmap loaded;

	if (networkOk && httpStatus >= 200 && httpStatus < 300) {
		loaded.loadFromData(body);
	}
	else {
		qWarning().noquote()
			<< QStringLiteral("[TitleBarUserChip] avatar download failed: status=%1, url=%2, message=%3")
			.arg(httpStatus)
			.arg(requestUrl, errorMessage.isEmpty() ? QStringLiteral("http request failed") : errorMessage);
	}

	if (networkOk && httpStatus >= 200 && httpStatus < 300 && loaded.isNull()) {
		qWarning().noquote()
			<< QStringLiteral("[TitleBarUserChip] avatar payload is not a valid image: status=%1, url=%2, bytes=%3")
			.arg(httpStatus)
			.arg(requestUrl)
			.arg(body.size());
	}

	if (loaded.isNull()) {
		ApplyFallbackAvatar();
		return;
	}

	ApplyAvatarIcon(MakeCircularAvatar(loaded));
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END

