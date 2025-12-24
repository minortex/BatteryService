# Maintainer: Your Name <youremail@example.com>
pkgname=battery-service-git
pkgver=r3.39d3b30 # 这是一个占位符，makepkg 运行后会自动更新它
pkgrel=1
pkgdesc="Battery monitoring service (Git version)"
arch=('x86_64')
url="https://github.com/minortex/BatteryService"
license=('GPL')
depends=('qt6-base' 'upower')
makedepends=('git' 'cmake' 'ninja')
provides=("${pkgname%-git}")
conflicts=("${pkgname%-git}")

# 使用 git 地址
source=("${pkgname}::git+${url}.git")
sha256sums=('SKIP')

pkgver() {
  cd "${pkgname}"
  # 获取提交次数和最新的短 hash (例如: r150.g1a2b3c4)
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cmake --preset linux-gcc-ninja-release -DCMAKE_INSTALL_PREFIX=/usr \
    cmake --build --preset linux-gcc-ninja-release-build
}

package() {
  DESTDIR="$pkgdir" cmake --install build/linux-gcc-ninja-release
}
