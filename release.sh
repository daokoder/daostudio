
make macosx

mkdir DaoStudio-$VERSION
mkdir DaoStudio-$VERSION/dao

cd DaoStudio-$VERSION && fossil open --nested ../DaoStudio.fossil && fossil close

cd dao && fossil open --nested ../../dao/Dao.fossil && fossil close

mkdir -p doc 
cp -r ../../dao/doc/html doc/

cd modules && fossil open --nested ../../../dao/modules/DaoModules.fossil && fossil close

cd ../tools && fossil open --nested ../../../dao/tools/DaoTools.fossil && fossil close


cd ../../../ && tar -zcf DaoStudio-$VERSION.tgz DaoStudio-$VERSION
