
make macosx

mkdir DaoStudio-$VERSION
mkdir DaoStudio-$VERSION/dao

cd DaoStudio-$VERSION && fossil open --nested ../DaoStudio.fossil && fossil close

cd dao && fossil open --nested ../../dao/Dao.fossil && fossil close

cd modules && fossil open --nested ../../../dao/modules/DaoModules.fossil && fossil close

cd ../tools && fossil open --nested ../../../dao/tools/DaoTools.fossil && fossil close

cd ../ && cp -r ../doc .

cd ../../ && tar -zcf DaoStudio-$VERSION.tgz DaoStudio-$VERSION
